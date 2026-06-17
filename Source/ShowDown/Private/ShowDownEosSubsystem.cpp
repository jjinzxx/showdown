#include "ShowDownEosSubsystem.h"

#include "Engine/Engine.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystem.h"
#include "SupabaseSubsystem.h"

namespace
{
constexpr int32 LocalUserNum = 0;
const FName ShowDownSessionName = NAME_GameSession;
const FName ShowDownRoomCodeKey = TEXT("SHOWDOWN_ROOM_CODE");
const FName ShowDownGameStartedKey = TEXT("SHOWDOWN_GAME_STARTED");
const FName ShowDownGameMapKey = TEXT("SHOWDOWN_GAME_MAP");
const FString EosOpenIdCredentialType = TEXT("externalauth:OpenIdAccessToken");

FString MakeRoomCode()
{
	return FString::Printf(TEXT("%06d"), FMath::RandRange(0, 999999));
}
}

IOnlineSubsystem* UShowDownEosSubsystem::GetEosSubsystem() const
{
	return IOnlineSubsystem::Get(FName(TEXT("EOS")));
}

IOnlineIdentityPtr UShowDownEosSubsystem::GetIdentityInterface() const
{
	if (IOnlineSubsystem* OnlineSubsystem = GetEosSubsystem())
	{
		return OnlineSubsystem->GetIdentityInterface();
	}

	return nullptr;
}

IOnlineSessionPtr UShowDownEosSubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* OnlineSubsystem = GetEosSubsystem())
	{
		return OnlineSubsystem->GetSessionInterface();
	}

	return nullptr;
}

bool UShowDownEosSubsystem::IsEosLoggedIn() const
{
	const IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();
	return IdentityInterface.IsValid() &&
		IdentityInterface->GetLoginStatus(LocalUserNum) == ELoginStatus::LoggedIn;
}

bool UShowDownEosSubsystem::IsInMultiplayerLobby() const
{
	return bInMultiplayerLobby;
}

bool UShowDownEosSubsystem::IsLobbyHost() const
{
	return bLobbyHost;
}

FString UShowDownEosSubsystem::GetLobbyCode() const
{
	return LobbyCode;
}

void UShowDownEosSubsystem::LoginWithSupabaseSession()
{
	if (IsEosLoggedIn())
	{
		OnEosLoginResult.Broadcast(true, TEXT("EOS login already active."));
		return;
	}

	USupabaseSubsystem* SupabaseSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<USupabaseSubsystem>()
		: nullptr;

	if (!SupabaseSubsystem || SupabaseSubsystem->GetAccessToken().IsEmpty())
	{
		OnEosLoginResult.Broadcast(false, TEXT("Supabase login is required before EOS login."));
		return;
	}

	const IOnlineIdentityPtr IdentityInterface = GetIdentityInterface();
	if (!IdentityInterface.IsValid())
	{
		OnEosLoginResult.Broadcast(false, TEXT("EOS identity interface is unavailable."));
		return;
	}

	if (LoginCompleteDelegateHandle.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegateHandle);
		LoginCompleteDelegateHandle.Reset();
	}

	LoginCompleteDelegateHandle = IdentityInterface->AddOnLoginCompleteDelegate_Handle(
		LocalUserNum,
		FOnLoginCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleLoginComplete)
	);

	FOnlineAccountCredentials Credentials;
	Credentials.Type = EosOpenIdCredentialType;
	Credentials.Id = SupabaseSubsystem->GetUserId();
	Credentials.Token = SupabaseSubsystem->GetAccessToken();

	OnEosLoginResult.Broadcast(false, TEXT("Logging in to EOS..."));

	if (!IdentityInterface->Login(LocalUserNum, Credentials))
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(LocalUserNum, LoginCompleteDelegateHandle);
		LoginCompleteDelegateHandle.Reset();
		OnEosLoginResult.Broadcast(false, TEXT("EOS login request could not start."));
	}
}

void UShowDownEosSubsystem::HostSession(FName MapName)
{
	if (!IsEosLoggedIn())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS login is required before hosting."));
		LoginWithSupabaseSession();
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return;
	}

	PendingHostMapName = MapName.IsNone() ? FName(TEXT("ShowDownRoom")) : MapName;
	PendingSessionFlow = ESessionFlow::HostImmediateGame;

	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleCreateSessionComplete)
	);

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = 4;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(SETTING_MAPNAME, PendingHostMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

	if (SessionInterface->GetNamedSession(ShowDownSessionName))
	{
		OnSessionResult.Broadcast(false, TEXT("An EOS session already exists. Destroy it before hosting again."));
		return;
	}

	OnSessionResult.Broadcast(false, TEXT("Creating EOS session..."));

	if (!SessionInterface->CreateSession(LocalUserNum, ShowDownSessionName, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		OnSessionResult.Broadcast(false, TEXT("EOS session creation could not start."));
	}
}

void UShowDownEosSubsystem::HostLobby(FName LobbyMapName, FName GameMapName)
{
	if (!IsEosLoggedIn())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS login is required before hosting."));
		LoginWithSupabaseSession();
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return;
	}

	if (SessionInterface->GetNamedSession(ShowDownSessionName))
	{
		OnSessionResult.Broadcast(false, TEXT("An EOS session already exists. Restart the app before hosting again."));
		return;
	}

	PendingHostMapName = LobbyMapName.IsNone() ? FName(TEXT("L_Hub")) : LobbyMapName;
	PendingGameMapName = GameMapName.IsNone() ? FName(TEXT("ShowDownRoom")) : GameMapName;
	LobbyCode = MakeRoomCode();
	PendingJoinCode.Empty();
	bLobbyHost = true;
	bInMultiplayerLobby = true;
	PendingSessionFlow = ESessionFlow::HostLobby;

	if (CreateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleCreateSessionComplete)
	);

	FOnlineSessionSettings Settings;
	Settings.bIsLANMatch = false;
	Settings.NumPublicConnections = 4;
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	Settings.Set(SETTING_MAPNAME, PendingHostMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(ShowDownRoomCodeKey, LobbyCode, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(ShowDownGameStartedKey, false, EOnlineDataAdvertisementType::ViaOnlineService);
	Settings.Set(ShowDownGameMapKey, PendingGameMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

	OnSessionResult.Broadcast(false, FString::Printf(TEXT("Creating room %s..."), *LobbyCode));

	if (!SessionInterface->CreateSession(LocalUserNum, ShowDownSessionName, Settings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
		PendingSessionFlow = ESessionFlow::None;
		bInMultiplayerLobby = false;
		bLobbyHost = false;
		LobbyCode.Empty();
		OnSessionResult.Broadcast(false, TEXT("EOS lobby creation could not start."));
	}
}

void UShowDownEosSubsystem::JoinLobbyByCode(const FString& RoomCode)
{
	if (!IsEosLoggedIn())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS login is required before joining."));
		LoginWithSupabaseSession();
		return;
	}

	const FString NormalizedCode = RoomCode.TrimStartAndEnd().ToUpper();
	if (NormalizedCode.IsEmpty())
	{
		OnSessionResult.Broadcast(false, TEXT("Enter a room code."));
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return;
	}

	PendingJoinCode = NormalizedCode;
	LobbyCode = NormalizedCode;
	bLobbyHost = false;
	PendingSessionFlow = ESessionFlow::JoinLobby;

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleFindSessionsComplete)
	);

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(ShowDownRoomCodeKey, NormalizedCode, EOnlineComparisonOp::Equals);

	OnSessionResult.Broadcast(false, FString::Printf(TEXT("Searching room %s..."), *NormalizedCode));

	if (!SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		PendingSessionFlow = ESessionFlow::None;
		OnSessionResult.Broadcast(false, TEXT("EOS lobby search could not start."));
	}
}

void UShowDownEosSubsystem::StartHostedGame()
{
	if (!bInMultiplayerLobby || !bLobbyHost)
	{
		OnSessionResult.Broadcast(false, TEXT("Only the room host can start the game."));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		OnSessionResult.Broadcast(false, TEXT("World is unavailable."));
		return;
	}

	int32 PlayerControllerCount = 0;
	for (FConstPlayerControllerIterator Iterator = World->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		++PlayerControllerCount;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Starting hosted game. NetMode=%d, PlayerControllers=%d, TargetMap=%s"),
		static_cast<int32>(World->GetNetMode()),
		PlayerControllerCount,
		*PendingGameMapName.ToString()
	);

	if (!GameMode)
	{
		OnSessionResult.Broadcast(false, TEXT("Only the listen server host can start the game."));
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	FNamedOnlineSession* NamedSession = SessionInterface.IsValid()
		? SessionInterface->GetNamedSession(ShowDownSessionName)
		: nullptr;

	if (!SessionInterface.IsValid() || !NamedSession)
	{
		OnSessionResult.Broadcast(true, TEXT("Starting game without EOS session update..."));
		TravelHostedGame();
		return;
	}

	if (UpdateSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteDelegateHandle);
		UpdateSessionCompleteDelegateHandle.Reset();
	}

	UpdateSessionCompleteDelegateHandle = SessionInterface->AddOnUpdateSessionCompleteDelegate_Handle(
		FOnUpdateSessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleUpdateSessionComplete)
	);

	FOnlineSessionSettings UpdatedSettings = NamedSession->SessionSettings;
	UpdatedSettings.Set(SETTING_MAPNAME, PendingGameMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);
	UpdatedSettings.Set(ShowDownGameStartedKey, true, EOnlineDataAdvertisementType::ViaOnlineService);
	UpdatedSettings.Set(ShowDownGameMapKey, PendingGameMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

	OnSessionResult.Broadcast(
		true,
		FString::Printf(TEXT("Starting game. Connected players: %d"), PlayerControllerCount)
	);

	if (!SessionInterface->UpdateSession(ShowDownSessionName, UpdatedSettings, true))
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteDelegateHandle);
		UpdateSessionCompleteDelegateHandle.Reset();
		OnSessionResult.Broadcast(true, TEXT("Session update skipped. Starting game..."));
		TravelHostedGame();
	}
}

void UShowDownEosSubsystem::StartLobbyStartPolling()
{
	if (!bInMultiplayerLobby || bLobbyHost || LobbyCode.IsEmpty())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyStartPollTimerHandle);
		World->GetTimerManager().SetTimer(
			LobbyStartPollTimerHandle,
			this,
			&UShowDownEosSubsystem::PollLobbyStart,
			1.0f,
			true,
			0.2f
		);
	}
}

void UShowDownEosSubsystem::StopLobbyStartPolling()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LobbyStartPollTimerHandle);
	}

	bLobbyStartPollInFlight = false;
}

void UShowDownEosSubsystem::FindAndJoinFirstSession()
{
	if (!IsEosLoggedIn())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS login is required before joining."));
		LoginWithSupabaseSession();
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return;
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleFindSessionsComplete)
	);

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 50;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	OnSessionResult.Broadcast(false, TEXT("Searching EOS sessions..."));

	if (!SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		OnSessionResult.Broadcast(false, TEXT("EOS session search could not start."));
	}
}

void UShowDownEosSubsystem::HandleLoginComplete(
	int32 InLocalUserNum,
	bool bWasSuccessful,
	const FUniqueNetId& UserId,
	const FString& Error
)
{
	if (const IOnlineIdentityPtr IdentityInterface = GetIdentityInterface(); IdentityInterface.IsValid())
	{
		IdentityInterface->ClearOnLoginCompleteDelegate_Handle(InLocalUserNum, LoginCompleteDelegateHandle);
		LoginCompleteDelegateHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		const FString Message = Error.IsEmpty()
			? TEXT("EOS login failed. Check EOS Connect/OpenID provider settings.")
			: FString::Printf(TEXT("EOS login failed: %s"), *Error);

		UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
		OnEosLoginResult.Broadcast(false, Message);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("EOS login success: %s"), *UserId.ToString());
	OnEosLoginResult.Broadcast(true, TEXT("EOS login success."));
}

void UShowDownEosSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		CreateSessionCompleteDelegateHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		PendingSessionFlow = ESessionFlow::None;
		bInMultiplayerLobby = false;
		bLobbyHost = false;
		OnSessionResult.Broadcast(false, TEXT("EOS session creation failed."));
		return;
	}

	const bool bOpeningLobby = PendingSessionFlow == ESessionFlow::HostLobby;
	OnSessionResult.Broadcast(
		true,
		bOpeningLobby
			? FString::Printf(TEXT("Room created. Code: %s"), *LobbyCode)
			: TEXT("EOS session created.")
	);

	if (UWorld* World = GetWorld())
	{
		UGameplayStatics::OpenLevel(World, PendingHostMapName, true, TEXT("listen"));
	}
}

void UShowDownEosSubsystem::HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnUpdateSessionCompleteDelegate_Handle(UpdateSessionCompleteDelegateHandle);
		UpdateSessionCompleteDelegateHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		OnSessionResult.Broadcast(true, TEXT("Session update failed, starting game anyway..."));
	}

	TravelHostedGame();
}

void UShowDownEosSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
	}

	const bool bPollingLobbyStart = PendingSessionFlow == ESessionFlow::PollLobbyStart;
	if (bPollingLobbyStart)
	{
		bLobbyStartPollInFlight = false;
	}

	if (!bWasSuccessful || !SessionSearch.IsValid() || SessionSearch->SearchResults.Num() == 0)
	{
		if (!bPollingLobbyStart)
		{
			OnSessionResult.Broadcast(false, TEXT("No EOS sessions found."));
		}
		return;
	}

	if (SessionInterface.IsValid())
	{
		int32 MatchIndex = 0;
		if (!PendingJoinCode.IsEmpty())
		{
			MatchIndex = INDEX_NONE;
			for (int32 Index = 0; Index < SessionSearch->SearchResults.Num(); ++Index)
			{
				FString FoundCode;
				SessionSearch->SearchResults[Index].Session.SessionSettings.Get(ShowDownRoomCodeKey, FoundCode);
				if (FoundCode.Equals(PendingJoinCode, ESearchCase::IgnoreCase))
				{
					MatchIndex = Index;
					break;
				}
			}

			if (MatchIndex == INDEX_NONE)
			{
				if (!bPollingLobbyStart)
				{
					OnSessionResult.Broadcast(false, FString::Printf(TEXT("Room %s was not found."), *PendingJoinCode));
				}
				return;
			}
		}

		if (bPollingLobbyStart)
		{
			bool bGameStarted = false;
			SessionSearch->SearchResults[MatchIndex].Session.SessionSettings.Get(ShowDownGameStartedKey, bGameStarted);

			if (bGameStarted)
			{
				FString GameMapName;
				SessionSearch->SearchResults[MatchIndex].Session.SessionSettings.Get(ShowDownGameMapKey, GameMapName);
				if (!GameMapName.IsEmpty())
				{
					PendingGameMapName = FName(*GameMapName);
				}

				StopLobbyStartPolling();
				bInMultiplayerLobby = false;
				PendingStartedGameSearchResult = SessionSearch->SearchResults[MatchIndex];
				JoinStartedGameSession();
			}
			return;
		}

		if (JoinSessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
			JoinSessionCompleteDelegateHandle.Reset();
		}

		JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleJoinSessionComplete)
		);

		OnSessionResult.Broadcast(false, TEXT("Joining EOS session..."));
		SessionInterface->JoinSession(LocalUserNum, ShowDownSessionName, SessionSearch->SearchResults[MatchIndex]);
	}
}

void UShowDownEosSubsystem::HandleJoinSessionComplete(
	FName SessionName,
	EOnJoinSessionCompleteResult::Type Result
)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	if (Result != EOnJoinSessionCompleteResult::Success || !SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session join failed."));
		return;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString) || ConnectString.IsEmpty())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS connect string was empty."));
		return;
	}

	if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		const bool bJoiningStartedGame = PendingSessionFlow == ESessionFlow::JoinStartedGame;
		bInMultiplayerLobby = !bJoiningStartedGame && (PendingSessionFlow == ESessionFlow::JoinLobby || bInMultiplayerLobby);
		bLobbyHost = false;
		if (!PendingJoinCode.IsEmpty())
		{
			LobbyCode = PendingJoinCode;
		}
		PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
		OnSessionResult.Broadcast(true, bJoiningStartedGame ? TEXT("EOS game joined.") : TEXT("EOS lobby joined."));
		return;
	}

	OnSessionResult.Broadcast(false, TEXT("Player controller was unavailable for travel."));
}

void UShowDownEosSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
		DestroySessionCompleteDelegateHandle.Reset();
	}

	if (!bWasSuccessful)
	{
		OnSessionResult.Broadcast(false, TEXT("Could not refresh EOS session before joining game."));
		return;
	}

	JoinStartedGameSession();
}

void UShowDownEosSubsystem::PollLobbyStart()
{
	if (bLobbyStartPollInFlight || !bInMultiplayerLobby || bLobbyHost || LobbyCode.IsEmpty())
	{
		return;
	}

	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (FindSessionsCompleteDelegateHandle.IsValid())
	{
		return;
	}

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleFindSessionsComplete)
	);

	PendingJoinCode = LobbyCode;
	PendingSessionFlow = ESessionFlow::PollLobbyStart;
	bLobbyStartPollInFlight = true;

	SessionSearch = MakeShared<FOnlineSessionSearch>();
	SessionSearch->MaxSearchResults = 100;
	SessionSearch->bIsLanQuery = false;
	SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	SessionSearch->QuerySettings.Set(ShowDownRoomCodeKey, LobbyCode, EOnlineComparisonOp::Equals);

	if (!SessionInterface->FindSessions(LocalUserNum, SessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		FindSessionsCompleteDelegateHandle.Reset();
		bLobbyStartPollInFlight = false;
	}
}

void UShowDownEosSubsystem::JoinStartedGameSession()
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return;
	}

	if (SessionInterface->GetNamedSession(ShowDownSessionName))
	{
		if (DestroySessionCompleteDelegateHandle.IsValid())
		{
			return;
		}

		DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleDestroySessionComplete)
		);

		OnSessionResult.Broadcast(true, TEXT("Host started. Refreshing connection..."));
		if (!SessionInterface->DestroySession(ShowDownSessionName))
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
			OnSessionResult.Broadcast(false, TEXT("Could not refresh EOS session before joining game."));
		}
		return;
	}

	if (JoinSessionCompleteDelegateHandle.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
	}

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UShowDownEosSubsystem::HandleJoinSessionComplete)
	);

	PendingSessionFlow = ESessionFlow::JoinStartedGame;
	OnSessionResult.Broadcast(true, TEXT("Host started. Joining game..."));
	if (!SessionInterface->JoinSession(LocalUserNum, ShowDownSessionName, PendingStartedGameSearchResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		JoinSessionCompleteDelegateHandle.Reset();
		OnSessionResult.Broadcast(false, TEXT("EOS game join could not start."));
	}
}

void UShowDownEosSubsystem::TravelHostedGame()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		OnSessionResult.Broadcast(false, TEXT("World is unavailable."));
		return;
	}

	AGameModeBase* GameMode = World->GetAuthGameMode();
	if (!GameMode)
	{
		OnSessionResult.Broadcast(false, TEXT("Only the listen server host can start the game."));
		return;
	}

	bInMultiplayerLobby = false;
	const FString TravelPath = FString::Printf(TEXT("/Game/Maps/%s"), *PendingGameMapName.ToString());
	OnSessionResult.Broadcast(true, TEXT("Traveling to game..."));
	GameMode->ProcessServerTravel(TravelPath, false);
}

bool UShowDownEosSubsystem::TravelToSearchResult(
	const FOnlineSessionSearchResult& SearchResult,
	const FString& StatusMessage
)
{
	const IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS session interface is unavailable."));
		return false;
	}

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SearchResult, NAME_GamePort, ConnectString) || ConnectString.IsEmpty())
	{
		OnSessionResult.Broadcast(false, TEXT("EOS connect string was empty."));
		return false;
	}

	APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	if (!PlayerController)
	{
		OnSessionResult.Broadcast(false, TEXT("Player controller was unavailable for travel."));
		return false;
	}

	OnSessionResult.Broadcast(true, StatusMessage);
	PlayerController->ClientTravel(ConnectString, TRAVEL_Absolute);
	return true;
}
