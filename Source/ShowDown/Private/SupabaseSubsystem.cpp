#include "SupabaseSubsystem.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Json.h"
#include "JsonUtilities.h"

namespace
{
FString MakeLoginFailureMessage(int32 StatusCode)
{
	if (StatusCode == 400 || StatusCode == 401)
	{
		return TEXT("ID or password is incorrect.");
	}

	if (StatusCode == 429)
	{
		return TEXT("Too many login attempts. Try again later.");
	}

	if (StatusCode >= 500)
	{
		return TEXT("Login server is unavailable. Try again later.");
	}

	return TEXT("Login failed. Check your connection and try again.");
}

FString MakeLeaderboardFailureMessage(int32 StatusCode)
{
	if (StatusCode == 401 || StatusCode == 403)
	{
		return TEXT("Please log in again to view rankings.");
	}

	if (StatusCode >= 500)
	{
		return TEXT("Ranking server is unavailable. Try again later.");
	}

	return TEXT("Could not load rankings. Check your connection and try again.");
}

FString MakeRewardFailureMessage(int32 StatusCode)
{
	if (StatusCode == 401 || StatusCode == 403)
	{
		return TEXT("Reward failed. Please log in again.");
	}

	if (StatusCode >= 500)
	{
		return TEXT("Reward server is unavailable. Your reward was not applied.");
	}

	return TEXT("Reward failed. Please try again later.");
}
}

void USupabaseSubsystem::LoginWithEmail(const FString& Email, const FString& Password)
{
	// 이메일이나 비밀번호가 비어 있으면 서버에 요청하지 않고 바로 실패 처리합니다.
	if (Email.IsEmpty() || Password.IsEmpty())
	{
		OnLoginResult.Broadcast(false, TEXT("Enter your email and password."));
		return;
	}

	// Unreal의 HTTP 모듈을 통해 Supabase 로그인 요청 객체를 만듭니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	// Supabase Auth의 이메일/비밀번호 로그인 API 주소입니다.
	const FString Url = SupabaseUrl + TEXT("/auth/v1/token?grant_type=password");

	Request->SetURL(Url);
	Request->SetVerb(TEXT("POST"));

	// anon public key를 apikey 헤더에 넣어 Supabase 프로젝트에 접근합니다.
	Request->SetHeader(TEXT("apikey"), SupabaseAnonKey);
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// Supabase 로그인 API가 요구하는 JSON body를 만듭니다.
	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetStringField(TEXT("email"), Email);
	BodyObject->SetStringField(TEXT("password"), Password);

	// JSON 객체를 문자열로 변환해서 HTTP body에 넣습니다.
	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);

	Request->SetContentAsString(BodyString);

	// 서버 응답이 돌아오면 HandleLoginResponse 함수가 호출되도록 연결합니다.
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleLoginResponse
	);

	// 실제 HTTP 요청을 시작합니다. 이 요청은 비동기로 처리됩니다.
	Request->ProcessRequest();

	// UI에 즉시 "로그인 중" 상태를 알려줍니다.
	OnLoginResult.Broadcast(false, TEXT("Logging in..."));
}

void USupabaseSubsystem::HandleLoginResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 네트워크 요청 자체가 실패했거나 응답 객체가 없으면 실패 처리합니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnLoginResult.Broadcast(false, TEXT("Network error. Check your connection and try again."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// 200번대가 아니면 Supabase 로그인 실패입니다.
	// 이메일/비밀번호 오류, URL 오류, 키 오류 등이 여기에 들어옵니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Supabase login failed: %d / %s"), StatusCode, *ResponseText);
		OnLoginResult.Broadcast(false, MakeLoginFailureMessage(StatusCode));
		return;
	}

	// Supabase가 돌려준 JSON 응답을 파싱합니다.
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OnLoginResult.Broadcast(false, TEXT("Login response parse failed."));
		return;
	}

	// 로그인 성공 응답에서 access_token을 꺼내 저장합니다.
	// 이후 DB 요청에서 Authorization: Bearer 토큰 형태로 사용합니다.
	if (!JsonObject->TryGetStringField(TEXT("access_token"), AccessToken))
	{
		OnLoginResult.Broadcast(false, TEXT("Access token not found."));
		return;
	}

	// 로그인 응답 안의 user 객체에서 유저 id를 꺼내 저장합니다.
	// 현재 DB 테이블들의 user_id와 같은 값입니다.
	const TSharedPtr<FJsonObject>* UserObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("user"), UserObject) && UserObject && UserObject->IsValid())
	{
		(*UserObject)->TryGetStringField(TEXT("id"), UserId);
	}

	UE_LOG(LogTemp, Log, TEXT("Supabase login success"));
	UE_LOG(LogTemp, Log, TEXT("UserId: %s"), *UserId);

	// 로그인에 성공했으므로 이어서 닉네임, coin, score를 불러옵니다.
	LoadPlayerData();

	// UI에 로그인 성공을 알립니다.
	OnLoginResult.Broadcast(true, TEXT("Login success."));
}

void USupabaseSubsystem::LoginWithId(const FString& Id, const FString& Password)
{
	// 아이디나 비밀번호가 비어 있으면 서버에 요청하지 않고 바로 실패 처리합니다.
	if (Id.IsEmpty() || Password.IsEmpty())
	{
		OnLoginResult.Broadcast(false, TEXT("Enter your ID and password."));
		return;
	}

	// login-with-id Edge Function을 호출합니다.
	// 아이디->이메일 매핑과 실제 로그인은 서버에서 처리되고, 응답으로 세션만 옵니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(SupabaseUrl + TEXT("/functions/v1/login-with-id"));
	Request->SetVerb(TEXT("POST"));

	// Edge Function 접근에도 anon 키가 필요합니다(아직 로그인 전이라 사용자 토큰은 없음).
	Request->SetHeader(TEXT("apikey"), SupabaseAnonKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *SupabaseAnonKey));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	// 요청 body: { "id": "...", "password": "..." }
	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetStringField(TEXT("id"), Id);
	BodyObject->SetStringField(TEXT("password"), Password);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleLoginWithIdResponse
	);

	Request->ProcessRequest();

	OnLoginResult.Broadcast(false, TEXT("Logging in..."));
}

void USupabaseSubsystem::HandleLoginWithIdResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnLoginResult.Broadcast(false, TEXT("Network error. Check your connection and try again."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// Edge Function은 실패 시 아이디 존재 여부를 숨기려고 항상 동일한 401을 돌려줍니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Login (id) failed: %d / %s"), StatusCode, *ResponseText);
		OnLoginResult.Broadcast(false, MakeLoginFailureMessage(StatusCode));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OnLoginResult.Broadcast(false, TEXT("Login response parse failed."));
		return;
	}

	// Edge Function이 돌려준 세션에서 access_token과 user_id를 저장합니다.
	if (!JsonObject->TryGetStringField(TEXT("access_token"), AccessToken))
	{
		OnLoginResult.Broadcast(false, TEXT("Access token not found."));
		return;
	}

	JsonObject->TryGetStringField(TEXT("user_id"), UserId);

	UE_LOG(LogTemp, Log, TEXT("Login (id) success. UserId: %s"), *UserId);

	// 로그인 성공 후 닉네임/coin/score를 불러옵니다.
	LoadPlayerData();

	OnLoginResult.Broadcast(true, TEXT("Login success."));
}

void USupabaseSubsystem::LoadLeaderboard(int32 Limit)
{
	if (AccessToken.IsEmpty())
	{
		OnLeaderboardLoaded.Broadcast(false, TEXT("Please log in again to view rankings."));
		return;
	}

	// get_leaderboard RPC 호출. 닉네임+점수만 정렬해서 돌려받습니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		CreateAuthorizedRequest(TEXT("/rest/v1/rpc/get_leaderboard"), TEXT("POST"));

	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetNumberField(TEXT("p_limit"), Limit);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleLeaderboardResponse
	);

	Request->ProcessRequest();
	OnLeaderboardLoaded.Broadcast(false, TEXT("Loading rankings..."));
}

void USupabaseSubsystem::HandleLeaderboardResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnLeaderboardLoaded.Broadcast(false, TEXT("Could not load rankings. Check your connection and try again."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Leaderboard load failed: %d / %s"), StatusCode, *ResponseText);
		OnLeaderboardLoaded.Broadcast(false, MakeLeaderboardFailureMessage(StatusCode));
		return;
	}

	// 응답 형태: [{"rank":1,"nickname":"a","score":1300}, ...]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnLeaderboardLoaded.Broadcast(false, TEXT("Ranking data could not be read. Try again later."));
		return;
	}

	Leaderboard.Reset();
	for (const TSharedPtr<FJsonValue>& Value : JsonArray)
	{
		const TSharedPtr<FJsonObject> Entry = Value->AsObject();
		if (!Entry.IsValid())
		{
			continue;
		}

		FShowDownRankEntry RankEntry;
		RankEntry.Rank = static_cast<int32>(Entry->GetIntegerField(TEXT("rank")));
		Entry->TryGetStringField(TEXT("nickname"), RankEntry.Nickname);
		RankEntry.Score = Entry->GetIntegerField(TEXT("score"));
		Leaderboard.Add(RankEntry);
	}

	UE_LOG(LogTemp, Log, TEXT("Leaderboard loaded: %d entries."), Leaderboard.Num());
	OnLeaderboardLoaded.Broadcast(true, TEXT("Leaderboard loaded."));
}

TArray<FShowDownRankEntry> USupabaseSubsystem::GetLeaderboard() const
{
	return Leaderboard;
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> USupabaseSubsystem::CreateAuthorizedRequest(
	const FString& Endpoint,
	const FString& Verb
)
{
	// 인증이 필요한 Supabase REST 요청을 만들 때 공통으로 사용하는 함수입니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();

	Request->SetURL(SupabaseUrl + Endpoint);
	Request->SetVerb(Verb);

	// Supabase REST API 요청에는 apikey와 Authorization 헤더가 필요합니다.
	Request->SetHeader(TEXT("apikey"), SupabaseAnonKey);
	Request->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *AccessToken));
	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	return Request;
}

void USupabaseSubsystem::LoadPlayerData()
{
	// access_token이 없으면 로그인하지 않은 상태이므로 DB 요청을 보내지 않습니다.
	if (AccessToken.IsEmpty())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Access token is empty."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading player data..."));

	// profiles 테이블에서 현재 로그인한 유저의 nickname을 가져옵니다.
	// RLS 정책 때문에 auth.uid()에 해당하는 자기 데이터만 반환됩니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> ProfileRequest =
		CreateAuthorizedRequest(TEXT("/rest/v1/profiles?select=nickname"), TEXT("GET"));

	ProfileRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleProfileResponse
	);

	ProfileRequest->ProcessRequest();

	// player_wallets 테이블에서 현재 유저의 coin을 가져옵니다.
	// rank GET과 동일하게 user_id 필터를 명시해 RLS 설정과 무관하게 본인 행만 읽습니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> WalletRequest =
		CreateAuthorizedRequest(
			FString::Printf(TEXT("/rest/v1/player_wallets?user_id=eq.%s&select=coin"), *UserId),
			TEXT("GET"));

	WalletRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleWalletResponse
	);

	WalletRequest->ProcessRequest();

	// player_ranks 테이블에서 현재 유저의 score를 가져옵니다.
	// user_id 필터를 명시해 RLS 설정과 무관하게 항상 본인 행만 읽도록 합니다.
	// (필터가 없으면 RLS가 느슨할 때 다른 행을 읽어 score가 엉뚱하게 나올 수 있습니다.)
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> RankRequest =
		CreateAuthorizedRequest(
			FString::Printf(TEXT("/rest/v1/player_ranks?user_id=eq.%s&select=score"), *UserId),
			TEXT("GET"));

	RankRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleRankResponse
	);

	RankRequest->ProcessRequest();
}

void USupabaseSubsystem::LoadCosmeticData()
{
	// 상점 상품은 skin_sets 기준으로 보여주고,
	// 실제 장착은 skin_set_items에 묶인 skins 기준으로 처리합니다.
	// 그래서 상품/보유 세트/세트 구성품/현재 장착값을 함께 불러와야 합니다.
	if (AccessToken.IsEmpty())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Access token is empty."));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Loading cosmetic data..."));

	ShopSkins.Empty();
	OwnedSkinIds.Empty();
	OwnedSkinSetIds.Empty();
	EquippedSkinIdsByType.Empty();
	SkinSetItemsBySetId.Empty();

	// 상점에 표시할 상품 목록입니다.
	// card와 card_back 같은 실제 스킨 조각은 skin_set_items에서 묶이고,
	// 플레이어에게는 skin_sets 한 줄이 하나의 상품으로 보입니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> SkinsRequest =
		CreateAuthorizedRequest(
			TEXT("/rest/v1/skin_sets?select=id,name,rarity,price,is_active,type&is_active=eq.true&order=price.asc"),
			TEXT("GET")
		);

	SkinsRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleSkinsResponse
	);

	SkinsRequest->ProcessRequest();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> PlayerSkinsRequest =
		CreateAuthorizedRequest(TEXT("/rest/v1/player_skins?select=skin_id"), TEXT("GET"));

	PlayerSkinsRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandlePlayerSkinsResponse
	);

	PlayerSkinsRequest->ProcessRequest();

	// 새 상점 구조의 보유 여부입니다.
	// 이전 구조 호환을 위해 player_skins도 읽지만, 상품 보유 판단은 이 테이블이 기준입니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> PlayerSkinSetsRequest =
		CreateAuthorizedRequest(TEXT("/rest/v1/player_skin_sets?select=set_id"), TEXT("GET"));

	PlayerSkinSetsRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandlePlayerSkinSetsResponse
	);

	PlayerSkinSetsRequest->ProcessRequest();

	// 상품 세트가 실제로 어떤 장착 slot들을 포함하는지 읽습니다.
	// 예: gold_card_set -> gold_card(card), gold_card_back(card_back)
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> SkinSetItemsRequest =
		CreateAuthorizedRequest(TEXT("/rest/v1/skin_set_items?select=set_id,skin_id,slot"), TEXT("GET"));

	SkinSetItemsRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleSkinSetItemsResponse
	);

	SkinSetItemsRequest->ProcessRequest();

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> EquipmentRequest =
		CreateAuthorizedRequest(TEXT("/rest/v1/player_equipment?select=skin_type,equipped_skin_id"), TEXT("GET"));

	EquipmentRequest->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandlePlayerEquipmentResponse
	);

	EquipmentRequest->ProcessRequest();
}

void USupabaseSubsystem::HandleProfileResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 프로필 요청 자체가 실패한 경우입니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Profile request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// Supabase REST 요청이 실패한 경우 응답 내용을 로그로 남깁니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Profile load failed: %d / %s"), StatusCode, *ResponseText);
		OnPlayerDataLoaded.Broadcast(false, TEXT("Profile load failed."));
		return;
	}

	// Supabase REST 응답은 배열 형태로 옵니다.
	// 예: [{"nickname":"Player_AB12"}]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Profile response parse failed."));
		return;
	}

	// 첫 번째 객체에서 nickname 값을 읽어 저장합니다.
	const TSharedPtr<FJsonObject> ProfileObject = JsonArray[0]->AsObject();

	if (ProfileObject.IsValid())
	{
		ProfileObject->TryGetStringField(TEXT("nickname"), Nickname);
	}

	UE_LOG(LogTemp, Log, TEXT("Nickname: %s"), *Nickname);
	OnPlayerDataLoaded.Broadcast(true, TEXT("Profile loaded."));
}

void USupabaseSubsystem::HandleWalletResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 재화 요청 자체가 실패한 경우입니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Wallet request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// REST API 상태 코드가 실패라면 로그로 내용을 확인합니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Wallet load failed: %d / %s"), StatusCode, *ResponseText);
		OnPlayerDataLoaded.Broadcast(false, TEXT("Wallet load failed."));
		return;
	}

	// 예: [{"coin":0}]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Wallet response parse failed."));
		return;
	}

	// 첫 번째 객체에서 coin 값을 읽어 저장합니다.
	const TSharedPtr<FJsonObject> WalletObject = JsonArray[0]->AsObject();

	if (WalletObject.IsValid())
	{
		Coin = WalletObject->GetIntegerField(TEXT("coin"));
	}

	UE_LOG(LogTemp, Log, TEXT("Coin: %d"), Coin);
	OnPlayerDataLoaded.Broadcast(true, TEXT("Wallet loaded."));
}

void USupabaseSubsystem::HandleRankResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 랭크 요청 자체가 실패한 경우입니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Rank request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// REST API 상태 코드가 실패라면 로그로 내용을 확인합니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Rank load failed: %d / %s"), StatusCode, *ResponseText);
		OnPlayerDataLoaded.Broadcast(false, TEXT("Rank load failed."));
		return;
	}

	// 예: [{"score":1000}]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Rank response parse failed."));
		return;
	}

	// 첫 번째 객체에서 score 값을 읽어 저장합니다.
	const TSharedPtr<FJsonObject> RankObject = JsonArray[0]->AsObject();

	if (RankObject.IsValid())
	{
		Score = RankObject->GetIntegerField(TEXT("score"));
	}

	UE_LOG(LogTemp, Log, TEXT("Score: %d"), Score);
	OnPlayerDataLoaded.Broadcast(true, TEXT("Rank loaded."));
}

void USupabaseSubsystem::HandleSkinsResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 함수 이름은 이전 skins 구조에서 이어졌지만,
	// 현재 응답은 skin_sets 테이블의 상점 상품 목록입니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skins request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skins load failed: %d / %s"), StatusCode, *ResponseText);
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skins load failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skins response parse failed."));
		return;
	}

	ShopSkins.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		const TSharedPtr<FJsonObject> SkinObject = JsonValue.IsValid()
			? JsonValue->AsObject()
			: nullptr;

		if (!SkinObject.IsValid())
		{
			continue;
		}

		FShowDownSkin Skin;
		SkinObject->TryGetStringField(TEXT("id"), Skin.Id);
		SkinObject->TryGetStringField(TEXT("name"), Skin.Name);
		SkinObject->TryGetStringField(TEXT("rarity"), Skin.Rarity);
		SkinObject->TryGetStringField(TEXT("type"), Skin.Type);
		SkinObject->TryGetBoolField(TEXT("is_active"), Skin.bIsActive);

		double Price = 0.0;
		if (SkinObject->TryGetNumberField(TEXT("price"), Price))
		{
			Skin.Price = FMath::RoundToInt(Price);
		}

		ShopSkins.Add(Skin);
	}

	UE_LOG(LogTemp, Log, TEXT("Shop skins loaded: %d"), ShopSkins.Num());
	OnCosmeticDataLoaded.Broadcast(true, TEXT("Shop skins loaded."));
}

void USupabaseSubsystem::HandlePlayerSkinsResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skins request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player skins load failed: %d / %s"), StatusCode, *ResponseText);
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skins load failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skins response parse failed."));
		return;
	}

	OwnedSkinIds.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		const TSharedPtr<FJsonObject> PlayerSkinObject = JsonValue.IsValid()
			? JsonValue->AsObject()
			: nullptr;

		if (!PlayerSkinObject.IsValid())
		{
			continue;
		}

		FString SkinId;
		if (PlayerSkinObject->TryGetStringField(TEXT("skin_id"), SkinId))
		{
			OwnedSkinIds.Add(SkinId);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Owned skins loaded: %d"), OwnedSkinIds.Num());
	OnCosmeticDataLoaded.Broadcast(true, TEXT("Player skins loaded."));
}

void USupabaseSubsystem::HandlePlayerSkinSetsResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skin sets request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Player skin sets load failed: %d / %s"), StatusCode, *ResponseText);
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skin sets load failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Player skin sets response parse failed."));
		return;
	}

	OwnedSkinSetIds.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		const TSharedPtr<FJsonObject> PlayerSkinSetObject = JsonValue.IsValid()
			? JsonValue->AsObject()
			: nullptr;

		if (!PlayerSkinSetObject.IsValid())
		{
			continue;
		}

		FString SetId;
		if (PlayerSkinSetObject->TryGetStringField(TEXT("set_id"), SetId))
		{
			OwnedSkinSetIds.Add(SetId);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Owned skin sets loaded: %d"), OwnedSkinSetIds.Num());
	OnCosmeticDataLoaded.Broadcast(true, TEXT("Player skin sets loaded."));
}

void USupabaseSubsystem::HandleSkinSetItemsResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skin set items request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Skin set items load failed: %d / %s"), StatusCode, *ResponseText);
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skin set items load failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Skin set items response parse failed."));
		return;
	}

	SkinSetItemsBySetId.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		const TSharedPtr<FJsonObject> SkinSetItemObject = JsonValue.IsValid()
			? JsonValue->AsObject()
			: nullptr;

		if (!SkinSetItemObject.IsValid())
		{
			continue;
		}

		FString SetId;
		FShowDownSkin SkinItem;

		if (
			SkinSetItemObject->TryGetStringField(TEXT("set_id"), SetId) &&
			SkinSetItemObject->TryGetStringField(TEXT("skin_id"), SkinItem.Id) &&
			SkinSetItemObject->TryGetStringField(TEXT("slot"), SkinItem.Type)
		)
		{
			SkinSetItemsBySetId.FindOrAdd(SetId).Add(SkinItem);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Skin set items loaded: %d"), SkinSetItemsBySetId.Num());
	OnCosmeticDataLoaded.Broadcast(true, TEXT("Skin set items loaded."));
}

void USupabaseSubsystem::HandlePlayerEquipmentResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Equipment request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Equipment load failed: %d / %s"), StatusCode, *ResponseText);
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Equipment load failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray))
	{
		OnCosmeticDataLoaded.Broadcast(false, TEXT("Equipment response parse failed."));
		return;
	}

	EquippedSkinIdsByType.Empty();

	for (const TSharedPtr<FJsonValue>& JsonValue : JsonArray)
	{
		const TSharedPtr<FJsonObject> EquipmentObject = JsonValue.IsValid()
			? JsonValue->AsObject()
			: nullptr;

		if (!EquipmentObject.IsValid())
		{
			continue;
		}

		FString SkinType;
		FString EquippedSkinId;

		if (
			EquipmentObject->TryGetStringField(TEXT("skin_type"), SkinType) &&
			EquipmentObject->TryGetStringField(TEXT("equipped_skin_id"), EquippedSkinId)
		)
		{
			EquippedSkinIdsByType.Add(SkinType, EquippedSkinId);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Equipment loaded: %d"), EquippedSkinIdsByType.Num());
	OnCosmeticDataLoaded.Broadcast(true, TEXT("Equipment loaded."));
}

FString USupabaseSubsystem::GetAccessToken() const
{
	// 현재 로그인 세션의 access token을 반환합니다.
	return AccessToken;
}

FString USupabaseSubsystem::GetUserId() const
{
	// 현재 로그인한 유저의 Supabase auth.users.id를 반환합니다.
	return UserId;
}

FString USupabaseSubsystem::GetNickname() const
{
	// 마지막으로 서버에서 불러온 닉네임을 반환합니다.
	return Nickname;
}

int32 USupabaseSubsystem::GetCoin() const
{
	// 마지막으로 서버에서 불러온 coin 값을 반환합니다.
	return Coin;
}

int32 USupabaseSubsystem::GetScore() const
{
	// 마지막으로 서버에서 불러온 score 값을 반환합니다.
	return Score;
}

TArray<FShowDownSkin> USupabaseSubsystem::GetShopSkins() const
{
	return ShopSkins;
}

TArray<FString> USupabaseSubsystem::GetOwnedSkinIds() const
{
	return OwnedSkinIds;
}

FString USupabaseSubsystem::GetEquippedSkinId(const FString& SkinType) const
{
	if (const FString* EquippedSkinId = EquippedSkinIdsByType.Find(SkinType))
	{
		return *EquippedSkinId;
	}

	return TEXT("");
}

bool USupabaseSubsystem::IsSkinOwned(const FString& SkinId) const
{
	return OwnedSkinSetIds.Contains(SkinId) || OwnedSkinIds.Contains(SkinId);
}

bool USupabaseSubsystem::IsShopItemEquipped(const FString& SetId) const
{
	// 세트 상품은 여러 실제 스킨을 포함할 수 있습니다.
	// 모든 구성 스킨이 현재 장착값과 일치할 때만 세트 전체가 장착된 상태로 봅니다.
	const TArray<FShowDownSkin>* SkinSetItems = SkinSetItemsBySetId.Find(SetId);

	if (!SkinSetItems || SkinSetItems->Num() == 0)
	{
		return false;
	}

	for (const FShowDownSkin& SkinItem : *SkinSetItems)
	{
		const FString* EquippedSkinId = EquippedSkinIdsByType.Find(SkinItem.Type);

		if (!EquippedSkinId || *EquippedSkinId != SkinItem.Id)
		{
			return false;
		}
	}

	return true;
}

void USupabaseSubsystem::EquipSkin(const FString& SkinId)
{
	// SkinId라는 이름은 기존 API 호환 때문에 유지하지만,
	// 현재 Shop에서는 skin_sets.id, 즉 상품 세트 id가 들어옵니다.
	if (AccessToken.IsEmpty())
	{
		OnSkinEquipped.Broadcast(false, TEXT("Access token is empty."));
		return;
	}

	if (UserId.IsEmpty())
	{
		OnSkinEquipped.Broadcast(false, TEXT("User id is empty."));
		return;
	}

	if (!IsSkinOwned(SkinId))
	{
		OnSkinEquipped.Broadcast(false, TEXT("Shop item is not owned."));
		return;
	}

	const TArray<FShowDownSkin>* SkinSetItems = SkinSetItemsBySetId.Find(SkinId);

	if (!SkinSetItems || SkinSetItems->Num() == 0)
	{
		OnSkinEquipped.Broadcast(false, TEXT("Shop item has no skin set items."));
		return;
	}

	for (const FShowDownSkin& SkinItem : *SkinSetItems)
	{
		// 세트 안의 각 실제 스킨을 slot별 장착값으로 저장합니다.
		// 카드 세트라면 card와 card_back PATCH 요청이 각각 나갑니다.
		if (SkinItem.Type.IsEmpty() || SkinItem.Id.IsEmpty())
		{
			continue;
		}

		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
			CreateAuthorizedRequest(
				FString::Printf(
					TEXT("/rest/v1/player_equipment?user_id=eq.%s&skin_type=eq.%s"),
					*UserId,
					*SkinItem.Type
				),
				TEXT("PATCH")
			);

		Request->SetHeader(TEXT("Prefer"), TEXT("return=representation"));

		TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
		BodyObject->SetStringField(TEXT("equipped_skin_id"), SkinItem.Id);

		FString BodyString;
		TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
		FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);

		Request->SetContentAsString(BodyString);
		Request->OnProcessRequestComplete().BindUObject(
			this,
			&USupabaseSubsystem::HandleEquipSkinResponse
		);

		Request->ProcessRequest();
	}

	OnSkinEquipped.Broadcast(false, TEXT("Equipping shop item..."));
}

void USupabaseSubsystem::PurchaseSkinSet(const FString& SetId)
{
	// 구매는 coin 차감, player_skin_sets 추가, player_skins 추가가 모두 맞물린 작업입니다.
	// 클라이언트에서 여러 REST 요청으로 나누지 않고 DB의 purchase_skin_set RPC에서 한 번에 처리합니다.
	if (AccessToken.IsEmpty())
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Access token is empty."));
		return;
	}

	if (SetId.IsEmpty())
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Shop item id is empty."));
		return;
	}

	if (IsSkinOwned(SetId))
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Shop item is already owned."));
		return;
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		CreateAuthorizedRequest(TEXT("/rest/v1/rpc/purchase_skin_set"), TEXT("POST"));

	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetStringField(TEXT("p_set_id"), SetId);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);

	Request->SetContentAsString(BodyString);
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandlePurchaseSkinSetResponse
	);

	Request->ProcessRequest();

	OnSkinSetPurchased.Broadcast(false, TEXT("Purchasing shop item..."));
}

void USupabaseSubsystem::UpdateNickname(const FString& NewNickname)
{
	// 입력값 앞뒤 공백을 제거해서 "  Player  " 같은 입력을 "Player"로 정리합니다.
	const FString TrimmedNickname = NewNickname.TrimStartAndEnd();

	// DB에도 2~16자 제한을 걸어두었으므로, 클라이언트에서도 먼저 검사합니다.
	// 이렇게 하면 잘못된 요청을 서버에 보내기 전에 UI에 바로 알려줄 수 있습니다.
	if (TrimmedNickname.Len() < 2 || TrimmedNickname.Len() > 16)
	{
		OnNicknameUpdated.Broadcast(false, TEXT("Nickname must be 2-16 characters."));
		return;
	}

	// 닉네임 변경은 로그인한 유저만 할 수 있으므로 access_token이 필요합니다.
	if (AccessToken.IsEmpty())
	{
		OnNicknameUpdated.Broadcast(false, TEXT("Access token is empty."));
		return;
	}

	// 어떤 유저의 profiles 행을 바꿀지 지정해야 하므로 UserId가 필요합니다.
	if (UserId.IsEmpty())
	{
		OnNicknameUpdated.Broadcast(false, TEXT("User id is empty."));
		return;
	}

	// profiles 테이블에서 현재 로그인한 유저 id와 일치하는 행만 PATCH로 수정합니다.
	// RLS 정책도 auth.uid() = id 조건이라, 자기 프로필만 수정할 수 있습니다.
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		CreateAuthorizedRequest(
			FString::Printf(TEXT("/rest/v1/profiles?id=eq.%s"), *UserId),
			TEXT("PATCH")
		);

	// 업데이트 후 변경된 행을 응답으로 돌려달라는 Supabase REST 옵션입니다.
	// 이걸 넣어야 응답에서 새 nickname을 다시 읽어올 수 있습니다.
	Request->SetHeader(TEXT("Prefer"), TEXT("return=representation"));

	// PATCH 요청 body에 들어갈 JSON을 만듭니다.
	// 결과 형태: { "nickname": "Player2" }
	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetStringField(TEXT("nickname"), TrimmedNickname);

	// JSON 객체를 문자열로 변환해서 HTTP body에 넣습니다.
	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);

	Request->SetContentAsString(BodyString);

	// 서버 응답이 돌아오면 HandleUpdateNicknameResponse가 실행되도록 연결합니다.
	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleUpdateNicknameResponse
	);

	// 실제 HTTP 요청을 시작합니다.
	Request->ProcessRequest();

	// UI에 즉시 "업데이트 중" 상태를 알려줍니다.
	OnNicknameUpdated.Broadcast(false, TEXT("Updating nickname..."));
}

void USupabaseSubsystem::HandleUpdateNicknameResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	// 네트워크 요청 자체가 실패했거나 응답 객체가 없으면 실패 처리합니다.
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnNicknameUpdated.Broadcast(false, TEXT("Nickname update request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	// 200번대가 아니면 Supabase에서 업데이트를 거절한 것입니다.
	// 닉네임 길이 제한, RLS 정책, 토큰 문제 등이 원인일 수 있습니다.
	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Nickname update failed: %d / %s"), StatusCode, *ResponseText);
		OnNicknameUpdated.Broadcast(false, TEXT("Nickname update failed."));
		return;
	}

	// Prefer: return=representation을 넣었기 때문에 응답은 배열 형태로 옵니다.
	// 예: [{"id":"...", "nickname":"Player2", ...}]
	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		OnNicknameUpdated.Broadcast(false, TEXT("Nickname update response parse failed."));
		return;
	}

	// 첫 번째 객체에서 변경된 nickname을 꺼내 내부 변수에 저장합니다.
	const TSharedPtr<FJsonObject> ProfileObject = JsonArray[0]->AsObject();

	if (ProfileObject.IsValid())
	{
		ProfileObject->TryGetStringField(TEXT("nickname"), Nickname);
	}

	// 로그로 실제 변경된 닉네임을 확인합니다.
	UE_LOG(LogTemp, Log, TEXT("Nickname updated: %s"), *Nickname);

	// UI에 닉네임 변경 성공을 알립니다.
	OnNicknameUpdated.Broadcast(true, TEXT("Nickname updated."));
}

void USupabaseSubsystem::AwardWinReward(const FString& MatchId)
{
	if (bAwardWinRewardInFlight)
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Reward is already being processed."));
		return;
	}

	if (MatchId.IsEmpty())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Reward failed. Match id is missing."));
		return;
	}

	// 보상 지급은 로그인한 유저만 가능하므로 access_token이 필요합니다.
	if (AccessToken.IsEmpty())
	{
		OnPlayerDataLoaded.Broadcast(false, TEXT("Reward failed. Please log in again."));
		return;
	}

	// 점수 보상은 award_win_reward RPC에서 서버 권한으로 처리합니다.
	// 보상 금액과 행 수정을 모두 서버가 결정/수행하므로 클라이언트는 score 값을 조작할 수 없습니다.
	// (SECURITY DEFINER 함수라 RLS UPDATE 정책 없이도 동작합니다.)
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request =
		CreateAuthorizedRequest(TEXT("/rest/v1/rpc/award_win_reward"), TEXT("POST"));

	TSharedPtr<FJsonObject> BodyObject = MakeShared<FJsonObject>();
	BodyObject->SetStringField(TEXT("p_match_id"), MatchId);

	FString BodyString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&BodyString);
	FJsonSerializer::Serialize(BodyObject.ToSharedRef(), Writer);
	Request->SetContentAsString(BodyString);

	Request->OnProcessRequestComplete().BindUObject(
		this,
		&USupabaseSubsystem::HandleAwardWinRewardResponse
	);

	Request->ProcessRequest();

	bAwardWinRewardInFlight = true;
	UE_LOG(LogTemp, Log, TEXT("AwardWinReward requested. MatchId=%s"), *MatchId);
	OnPlayerDataLoaded.Broadcast(false, TEXT("Processing win reward..."));
}

void USupabaseSubsystem::HandleAwardWinRewardResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		bAwardWinRewardInFlight = false;
		OnPlayerDataLoaded.Broadcast(false, TEXT("Reward failed. Check your connection and try again."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Reward failed: %d / %s"), StatusCode, *ResponseText);
		bAwardWinRewardInFlight = false;
		OnPlayerDataLoaded.Broadcast(false, MakeRewardFailureMessage(StatusCode));
		return;
	}

	// 응답 형태: { "reward": 30, "score": 1230, "coin_reward": 70, "coin": 520 }
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		bAwardWinRewardInFlight = false;
		OnPlayerDataLoaded.Broadcast(false, TEXT("Reward response could not be read. Please check your score later."));
		return;
	}

	int32 ScoreReward = 0;
	JsonObject->TryGetNumberField(TEXT("reward"), ScoreReward);

	int32 NewScore = Score;
	if (JsonObject->TryGetNumberField(TEXT("score"), NewScore))
	{
		Score = NewScore;
	}

	int32 CoinReward = 0;
	JsonObject->TryGetNumberField(TEXT("coin_reward"), CoinReward);

	int32 NewCoin = Coin;
	if (JsonObject->TryGetNumberField(TEXT("coin"), NewCoin))
	{
		Coin = NewCoin;
	}

	bool bAlreadyClaimed = false;
	JsonObject->TryGetBoolField(TEXT("already_claimed"), bAlreadyClaimed);

	UE_LOG(LogTemp, Log, TEXT("Win reward granted: +%d score (=%d), +%d coin (=%d)"),
		ScoreReward, Score, CoinReward, Coin);

	// 메인메뉴 등 UI가 점수/코인 표시를 새로고침하도록 알립니다.
	bAwardWinRewardInFlight = false;
	OnPlayerDataLoaded.Broadcast(
		true,
		bAlreadyClaimed ? TEXT("Win reward was already claimed.") : TEXT("Win reward granted.")
	);
}

void USupabaseSubsystem::HandleEquipSkinResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnSkinEquipped.Broadcast(false, TEXT("Equip skin request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Equip skin failed: %d / %s"), StatusCode, *ResponseText);
		OnSkinEquipped.Broadcast(false, TEXT("Equip skin failed."));
		return;
	}

	TArray<TSharedPtr<FJsonValue>> JsonArray;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonArray) || JsonArray.Num() == 0)
	{
		OnSkinEquipped.Broadcast(false, TEXT("Equip skin response parse failed."));
		return;
	}

	const TSharedPtr<FJsonObject> EquipmentObject = JsonArray[0]->AsObject();

	if (!EquipmentObject.IsValid())
	{
		OnSkinEquipped.Broadcast(false, TEXT("Equip skin response was empty."));
		return;
	}

	FString SkinType;
	FString EquippedSkinId;

	if (
		!EquipmentObject->TryGetStringField(TEXT("skin_type"), SkinType) ||
		!EquipmentObject->TryGetStringField(TEXT("equipped_skin_id"), EquippedSkinId)
	)
	{
		OnSkinEquipped.Broadcast(false, TEXT("Equip skin fields were missing."));
		return;
	}

	EquippedSkinIdsByType.Add(SkinType, EquippedSkinId);

	UE_LOG(LogTemp, Log, TEXT("Skin equipped: %s -> %s"), *SkinType, *EquippedSkinId);
	OnSkinEquipped.Broadcast(true, TEXT("Skin equipped."));
}

void USupabaseSubsystem::HandlePurchaseSkinSetResponse(
	FHttpRequestPtr Request,
	FHttpResponsePtr Response,
	bool bWasSuccessful
)
{
	if (!bWasSuccessful || !Response.IsValid())
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Purchase request failed."));
		return;
	}

	const int32 StatusCode = Response->GetResponseCode();
	const FString ResponseText = Response->GetContentAsString();

	if (StatusCode < 200 || StatusCode >= 300)
	{
		UE_LOG(LogTemp, Warning, TEXT("Purchase failed: %d / %s"), StatusCode, *ResponseText);
		OnSkinSetPurchased.Broadcast(false, TEXT("Purchase failed."));
		return;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseText);

	if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Purchase response parse failed."));
		return;
	}

	bool bSuccess = false;
	JsonObject->TryGetBoolField(TEXT("success"), bSuccess);

	if (!bSuccess)
	{
		OnSkinSetPurchased.Broadcast(false, TEXT("Purchase was not successful."));
		return;
	}

	double UpdatedCoin = 0.0;
	if (JsonObject->TryGetNumberField(TEXT("coin"), UpdatedCoin))
	{
		Coin = FMath::RoundToInt(UpdatedCoin);
	}

	FString PurchasedSetId;
	JsonObject->TryGetStringField(TEXT("set_id"), PurchasedSetId);

	UE_LOG(LogTemp, Log, TEXT("Shop item purchased: %s / coin=%d"), *PurchasedSetId, Coin);

	// RPC가 DB를 갱신했으므로 클라이언트 캐시도 다시 맞춥니다.
	LoadPlayerData();
	LoadCosmeticData();

	OnSkinSetPurchased.Broadcast(true, TEXT("Shop item purchased."));
}
