#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ShowDownEosSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FOnShowDownEosResult,
	bool,
	bSuccess,
	const FString&,
	Message
);

UCLASS()
class SHOWDOWN_API UShowDownEosSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "ShowDown|EOS")
	FOnShowDownEosResult OnEosLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "ShowDown|EOS")
	FOnShowDownEosResult OnSessionResult;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	bool IsEosLoggedIn() const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void LoginWithSupabaseSession();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void HostSession(FName MapName = TEXT("ShowDownRoom"));

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void FindAndJoinFirstSession();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void HostLobby(FName LobbyMapName = TEXT("L_MultiplayerLobby"), FName GameMapName = TEXT("ShowDownRoom"));

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void JoinLobbyByCode(const FString& RoomCode);

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void StartHostedGame();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void StartLobbyStartPolling();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	void StopLobbyStartPolling();

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	bool IsInMultiplayerLobby() const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	bool IsLobbyHost() const;

	UFUNCTION(BlueprintCallable, Category = "ShowDown|EOS")
	FString GetLobbyCode() const;

private:
	enum class ESessionFlow
	{
		None,
		HostImmediateGame,
		HostLobby,
		JoinLobby,
		PollLobbyStart,
		JoinStartedGame
	};

	FDelegateHandle LoginCompleteDelegateHandle;
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle UpdateSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;

	TSharedPtr<FOnlineSessionSearch> SessionSearch;
	FOnlineSessionSearchResult PendingStartedGameSearchResult;
	FTimerHandle LobbyStartPollTimerHandle;
	FName PendingHostMapName = TEXT("ShowDownRoom");
	FName PendingGameMapName = TEXT("ShowDownRoom");
	FString PendingJoinCode;
	FString LobbyCode;
	ESessionFlow PendingSessionFlow = ESessionFlow::None;
	bool bLobbyStartPollInFlight = false;
	bool bInMultiplayerLobby = false;
	bool bLobbyHost = false;

	class IOnlineSubsystem* GetEosSubsystem() const;
	IOnlineIdentityPtr GetIdentityInterface() const;
	IOnlineSessionPtr GetSessionInterface() const;

	void HandleLoginComplete(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleUpdateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void PollLobbyStart();
	void JoinStartedGameSession();
	void TravelHostedGame();
	bool TravelToSearchResult(const FOnlineSessionSearchResult& SearchResult, const FString& StatusMessage);
};
