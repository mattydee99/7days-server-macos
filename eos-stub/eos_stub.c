/*
 * eos_stub.c — stub replacement for libEOSSDK-Linux-Shipping.so
 *
 * Purpose: the real EOS SDK hangs indefinitely on ARM emulation (box64,
 * FEX-Emu, Rosetta-in-Linux-VM) because its async callbacks rely on x86_64
 * threading semantics that don't survive emulation. This stub exports all
 * 668 EOS SDK symbols and fires async completion callbacks immediately with
 * EOS_NoConnection, so the server skips EOS features and starts normally.
 *
 * Crossplay and Epic account features will be unavailable — that's intentional.
 *
 * Build (x86_64):
 *   gcc -m64 -shared -fPIC -nostartfiles \
 *       -Wl,--version-script=eos.map \
 *       -o libEOSSDK-Linux-Shipping.so eos_stub.c
 */

#include <stdint.h>
#include <string.h>

/* -------------------------------------------------------------------------
 * Fake handle: returned for all platform / interface / object handles.
 * Must be non-NULL so callers don't abort on null-checks.
 * -------------------------------------------------------------------------*/
static char _fake[64];

/* -------------------------------------------------------------------------
 * EOS result codes (from eos_result.h)
 * -------------------------------------------------------------------------*/
#define EOS_Success         0
#define EOS_NoConnection    4099   /* server is offline / can't reach Epic */

/* -------------------------------------------------------------------------
 * Generic async callback helper.
 *
 * Every EOS async completion callback struct starts with:
 *   EOS_EResult  ResultCode;   // int32
 *   void        *ClientData;   // pointer passed by caller
 *
 * We fire all of them immediately with EOS_NoConnection so the game
 * doesn't spin on EOS_Platform_Tick waiting forever.
 * -------------------------------------------------------------------------*/
typedef void (*eos_cb)(const void *);

static void _fire(eos_cb cb, void *clientdata, int result)
{
    if (!cb) return;
    struct { int result; void *clientdata; } data;
    data.result     = result;
    data.clientdata = clientdata;
    cb(&data);
}

/* -------------------------------------------------------------------------
 * Top-level SDK init / shutdown
 * -------------------------------------------------------------------------*/
int   EOS_Initialize(const void *o)          { (void)o; return EOS_Success; }
int   EOS_Shutdown(void)                     { return EOS_Success; }
const char *EOS_GetVersion(void)             { return "1.0.0-stub"; }
int   EOS_EResult_IsOperationComplete(int r) { return r != 1 /* EOS_OperationStarting */; }
const char *EOS_EResult_ToString(int r)      { (void)r; return "stub"; }
int   EOS_BeginScopeEvent(void)              { return EOS_Success; }
int   EOS_EndScopeEvent(void)                { return EOS_Success; }
const char *EOS_EApplicationStatus_ToString(int s) { (void)s; return "stub"; }
const char *EOS_ENetworkStatus_ToString(int s)     { (void)s; return "stub"; }

/* Logging */
int EOS_Logging_SetCallback(const void *o)    { (void)o; return EOS_Success; }
int EOS_Logging_SetLogLevel(int c, int l)     { (void)c; (void)l; return EOS_Success; }

/* Mercury (internal Epic subsystem) */
int  EOS_Mercury_Initialize(const void *o)   { (void)o; return EOS_Success; }
int  EOS_Mercury_Shutdown(void)              { return EOS_Success; }
void EOS_Mercury_Tick(void *h)               { (void)h; }

/* ByteArray / ID helpers */
int EOS_ByteArray_ToString(const void *a, void *b, uint32_t *s) { (void)a;(void)b;(void)s; return EOS_Success; }
int EOS_ContinuanceToken_ToString(void *t, void *b, uint32_t *s){ (void)t;(void)b;(void)s; return EOS_Success; }

/* EpicAccountId */
void *EOS_EpicAccountId_FromString(const char *s) { (void)s; return NULL; }
int   EOS_EpicAccountId_IsValid(void *id)         { (void)id; return 0; }
int   EOS_EpicAccountId_ToString(void *id, void *b, uint32_t *s){ (void)id;(void)b;(void)s; return EOS_Success; }

/* ProductUserId */
void *EOS_ProductUserId_FromString(const char *s) { (void)s; return NULL; }
int   EOS_ProductUserId_IsValid(void *id)         { (void)id; return 1; }
int   EOS_ProductUserId_ToString(void *id, void *b, uint32_t *s){ (void)id;(void)b;(void)s; return EOS_Success; }

/* -------------------------------------------------------------------------
 * Platform
 * -------------------------------------------------------------------------*/
void *EOS_Platform_Create(const void *o)                       { (void)o; return _fake; }
void  EOS_Platform_Tick(void *h)                               { (void)h; }
void  EOS_Platform_Release(void *h)                            { (void)h; }
int   EOS_Platform_CheckForLauncherAndRestart(void *h)         { (void)h; return EOS_Success; }
int   EOS_Platform_GetApplicationStatus(void *h)               { (void)h; return 0; }
int   EOS_Platform_SetApplicationStatus(void *h, int s)        { (void)h;(void)s; return EOS_Success; }
int   EOS_Platform_GetNetworkStatus(void *h)                   { (void)h; return 0; }
int   EOS_Platform_SetNetworkStatus(void *h, int s)            { (void)h;(void)s; return EOS_Success; }
int   EOS_Platform_GetActiveCountryCode(void *h, void *u, void *b, uint32_t *s)  { (void)h;(void)u;(void)b;(void)s; return EOS_Success; }
int   EOS_Platform_GetActiveLocaleCode(void *h, void *u, void *b, uint32_t *s)   { (void)h;(void)u;(void)b;(void)s; return EOS_Success; }
int   EOS_Platform_GetOverrideCountryCode(void *h, void *b, uint32_t *s)         { (void)h;(void)b;(void)s; return EOS_Success; }
int   EOS_Platform_SetOverrideCountryCode(void *h, const char *c)                { (void)h;(void)c; return EOS_Success; }
int   EOS_Platform_GetOverrideLocaleCode(void *h, void *b, uint32_t *s)          { (void)h;(void)b;(void)s; return EOS_Success; }
int   EOS_Platform_SetOverrideLocaleCode(void *h, const char *l)                 { (void)h;(void)l; return EOS_Success; }
int   EOS_Platform_GetDesktopCrossplayStatus(void *h, void *o)                   { (void)h;(void)o; return EOS_Success; }

/* Platform interface getters — all return fake non-null handles */
void *EOS_Platform_GetAchievementsInterface(void *h)           { (void)h; return _fake; }
void *EOS_Platform_GetAntiCheatClientInterface(void *h)        { (void)h; return _fake; }
void *EOS_Platform_GetAntiCheatServerInterface(void *h)        { (void)h; return _fake; }
void *EOS_Platform_GetAuthInterface(void *h)                   { (void)h; return _fake; }
void *EOS_Platform_GetConnectInterface(void *h)                { (void)h; return _fake; }
void *EOS_Platform_GetCustomInvitesInterface(void *h)          { (void)h; return _fake; }
void *EOS_Platform_GetEcomInterface(void *h)                   { (void)h; return _fake; }
void *EOS_Platform_GetFriendsInterface(void *h)                { (void)h; return _fake; }
void *EOS_Platform_GetIntegratedPlatformInterface(void *h)     { (void)h; return _fake; }
void *EOS_Platform_GetKWSInterface(void *h)                    { (void)h; return _fake; }
void *EOS_Platform_GetLeaderboardsInterface(void *h)           { (void)h; return _fake; }
void *EOS_Platform_GetLobbyInterface(void *h)                  { (void)h; return _fake; }
void *EOS_Platform_GetMetricsInterface(void *h)                { (void)h; return _fake; }
void *EOS_Platform_GetModsInterface(void *h)                   { (void)h; return _fake; }
void *EOS_Platform_GetP2PInterface(void *h)                    { (void)h; return _fake; }
void *EOS_Platform_GetPlayerDataStorageInterface(void *h)      { (void)h; return _fake; }
void *EOS_Platform_GetPresenceInterface(void *h)               { (void)h; return _fake; }
void *EOS_Platform_GetProgressionSnapshotInterface(void *h)    { (void)h; return _fake; }
void *EOS_Platform_GetReportsInterface(void *h)                { (void)h; return _fake; }
void *EOS_Platform_GetRTCAdminInterface(void *h)               { (void)h; return _fake; }
void *EOS_Platform_GetRTCInterface(void *h)                    { (void)h; return _fake; }
void *EOS_Platform_GetSanctionsInterface(void *h)              { (void)h; return _fake; }
void *EOS_Platform_GetSessionsInterface(void *h)               { (void)h; return _fake; }
void *EOS_Platform_GetStatsInterface(void *h)                  { (void)h; return _fake; }
void *EOS_Platform_GetTitleStorageInterface(void *h)           { (void)h; return _fake; }
void *EOS_Platform_GetUIInterface(void *h)                     { (void)h; return _fake; }
void *EOS_Platform_GetUserInfoInterface(void *h)               { (void)h; return _fake; }

/* RTC sub-interfaces */
void *EOS_RTC_GetAudioInterface(void *h)                       { (void)h; return _fake; }
void *EOS_RTC_GetDataInterface(void *h)                        { (void)h; return _fake; }

/* -------------------------------------------------------------------------
 * Auth
 * -------------------------------------------------------------------------*/
void EOS_Auth_Login(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_Logout(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_LinkAccount(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_DeletePersistentAuth(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_QueryIdToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_VerifyIdToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Auth_VerifyUserAuth(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Auth_GetLoggedInAccountsCount(void *h)                { (void)h; return 0; }
void *EOS_Auth_GetLoggedInAccountByIndex(void *h, int i)       { (void)h;(void)i; return NULL; }
int  EOS_Auth_GetLoginStatus(void *h, void *id)                { (void)h;(void)id; return 0; }
void *EOS_Auth_GetSelectedAccountId(void *h)                   { (void)h; return NULL; }
int  EOS_Auth_GetMergedAccountsCount(void *h, void *id)        { (void)h;(void)id; return 0; }
void *EOS_Auth_GetMergedAccountByIndex(void *h, void *id, int i){ (void)h;(void)id;(void)i; return NULL; }
int  EOS_Auth_CopyUserAuthToken(void *h, const void *o, void *id, void **t){ (void)h;(void)o;(void)id; if(t)*t=NULL; return EOS_Success; }
int  EOS_Auth_CopyIdToken(void *h, const void *o, void **t)    { (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
void EOS_Auth_Token_Release(void *t)                           { (void)t; }
void EOS_Auth_IdToken_Release(void *t)                         { (void)t; }
uint64_t EOS_Auth_AddNotifyLoginStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Auth_RemoveNotifyLoginStatusChanged(void *h, uint64_t id)
    { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * Connect
 * -------------------------------------------------------------------------*/
void EOS_Connect_Login(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_Logout(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_CreateUser(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_LinkAccount(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_UnlinkAccount(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_CreateDeviceId(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_DeleteDeviceId(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_TransferDeviceIdAccount(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_QueryExternalAccountMappings(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_QueryProductUserIdMappings(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Connect_VerifyIdToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
/* The original SDK's EOS_Connect_Login callback returns a LocalUserId.
   Returning a fake success with no LocalUserId means callers that try
   to read it will get a null ProductUserId.  That is intentional:
   crossplay/EOS joining is unsupported; we just want the server to
   believe its session registration succeeded. */
int  EOS_Connect_GetLoggedInUsersCount(void *h)                { (void)h; return 0; }
void *EOS_Connect_GetLoggedInUserByIndex(void *h, int i)       { (void)h;(void)i; return NULL; }
int  EOS_Connect_GetLoginStatus(void *h, void *id)             { (void)h;(void)id; return 0; }
int  EOS_Connect_GetProductUserExternalAccountCount(void *h, const void *o) { (void)h;(void)o; return 0; }
int  EOS_Connect_CopyProductUserExternalAccountByIndex(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_Connect_CopyProductUserExternalAccountByAccountType(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_Connect_CopyProductUserExternalAccountByAccountId(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_Connect_CopyProductUserInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
void EOS_Connect_ExternalAccountInfo_Release(void *i)          { (void)i; }
int  EOS_Connect_GetExternalAccountMapping(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Connect_GetProductUserIdMapping(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Connect_CopyIdToken(void *h, const void *o, void **t) { (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
void EOS_Connect_IdToken_Release(void *t)                      { (void)t; }
uint64_t EOS_Connect_AddNotifyAuthExpiration(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Connect_RemoveNotifyAuthExpiration(void *h, uint64_t id)
    { (void)h;(void)id; }
uint64_t EOS_Connect_AddNotifyLoginStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Connect_RemoveNotifyLoginStatusChanged(void *h, uint64_t id)
    { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * Sessions
 * -------------------------------------------------------------------------*/
/* NOTE: these used to fire EOS_NoConnection, but the dedicated server uses
 *       EOS_Sessions_UpdateSession as a fire-and-forget registration step.
 *       Returning "no connection" produces a negative result code and the
 *       server never becomes "ready".  Returning EOS_Success lets the server
 *       think the session was created/updated; player auth still fails later
 *       because we have no real accounts, but crossplay/EOS joining was
 *       already unsupported on this stub. */
int EOS_Sessions_CreateSessionModification(void *h, const void *o, void **m)
    { (void)h;(void)o; if(m)*m=_fake; return EOS_Success; }
int EOS_Sessions_UpdateSessionModification(void *h, const void *o, void **m)
    { (void)h;(void)o; if(m)*m=_fake; return EOS_Success; }
void EOS_Sessions_UpdateSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_DestroySession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_JoinSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_StartSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_EndSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_RegisterPlayers(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_UnregisterPlayers(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_SendInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_RejectInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sessions_QueryInvites(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Sessions_GetInviteCount(void *h, const void *o)       { (void)h;(void)o; return 0; }
int  EOS_Sessions_GetInviteIdByIndex(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Sessions_CreateSessionSearch(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=_fake; return EOS_Success; }
int  EOS_Sessions_DumpSessionState(void *h, const char *n)     { (void)h;(void)n; return EOS_Success; }
int  EOS_Sessions_IsUserInSession(void *h, const char *n, void *id){ (void)h;(void)n;(void)id; return 0; }
int  EOS_Sessions_CopyActiveSessionHandle(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_Sessions_CopySessionHandleByInviteId(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_Sessions_CopySessionHandleByUiEventId(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_Sessions_CopySessionHandleForPresence(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
uint64_t EOS_Sessions_AddNotifySessionInviteReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Sessions_AddNotifySessionInviteAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Sessions_AddNotifySessionInviteRejected(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Sessions_AddNotifyJoinSessionAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Sessions_AddNotifyLeaveSessionRequested(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Sessions_AddNotifySendSessionNativeInviteRequested(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Sessions_RemoveNotifySessionInviteReceived(void *h, uint64_t id)   { (void)h;(void)id; }
void EOS_Sessions_RemoveNotifySessionInviteAccepted(void *h, uint64_t id)   { (void)h;(void)id; }
void EOS_Sessions_RemoveNotifySessionInviteRejected(void *h, uint64_t id)   { (void)h;(void)id; }
void EOS_Sessions_RemoveNotifyJoinSessionAccepted(void *h, uint64_t id)     { (void)h;(void)id; }
void EOS_Sessions_RemoveNotifyLeaveSessionRequested(void *h, uint64_t id)   { (void)h;(void)id; }
void EOS_Sessions_RemoveNotifySendSessionNativeInviteRequested(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_SessionModification_Release(void *m)                  { (void)m; }
int  EOS_SessionModification_SetBucketId(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetMaxPlayers(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetPermissionLevel(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetJoinInProgressAllowed(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetInvitesAllowed(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetHostAddress(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_AddAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_RemoveAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_SessionModification_SetAllowedPlatformIds(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
void EOS_SessionDetails_Release(void *h)                        { (void)h; }
void EOS_SessionDetails_Info_Release(void *i)                   { (void)i; }
void EOS_SessionDetails_Attribute_Release(void *a)              { (void)a; }
int  EOS_SessionDetails_CopyInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_SessionDetails_GetSessionAttributeCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_SessionDetails_CopySessionAttributeByIndex(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_SessionDetails_CopySessionAttributeByKey(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
void EOS_ActiveSession_Release(void *h)                         { (void)h; }
void EOS_ActiveSession_Info_Release(void *i)                    { (void)i; }
int  EOS_ActiveSession_CopyInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_ActiveSession_GetRegisteredPlayerCount(void *h, const void *o){ (void)h;(void)o; return 0; }
void *EOS_ActiveSession_GetRegisteredPlayerByIndex(void *h, const void *o){ (void)h;(void)o; return NULL; }
void EOS_SessionSearch_Release(void *h)                         { (void)h; }
void EOS_SessionSearch_Find(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_SessionSearch_GetSearchResultCount(void *h)            { (void)h; return 0; }
int  EOS_SessionSearch_CopySearchResultByIndex(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_SessionSearch_SetSessionId(void *h, const void *o)    { (void)h;(void)o; return EOS_Success; }
int  EOS_SessionSearch_SetTargetUserId(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
int  EOS_SessionSearch_SetParameter(void *h, const void *o)    { (void)h;(void)o; return EOS_Success; }
int  EOS_SessionSearch_RemoveParameter(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
int  EOS_SessionSearch_SetMaxResults(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }

/* -------------------------------------------------------------------------
 * Lobby
 * -------------------------------------------------------------------------*/
void EOS_Lobby_CreateLobby(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_DestroyLobby(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_JoinLobby(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_JoinLobbyById(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_LeaveLobby(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_UpdateLobby(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_PromoteMember(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_KickMember(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_SendInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_RejectInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_QueryInvites(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_HardMuteMember(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_JoinRTCRoom(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Lobby_LeaveRTCRoom(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Lobby_UpdateLobbyModification(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=_fake; return EOS_Success; }
int  EOS_Lobby_CreateLobbySearch(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=_fake; return EOS_Success; }
int  EOS_Lobby_GetInviteCount(void *h, const void *o)          { (void)h;(void)o; return 0; }
int  EOS_Lobby_GetInviteIdByIndex(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Lobby_CopyLobbyDetailsHandle(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Lobby_CopyLobbyDetailsHandleByInviteId(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Lobby_CopyLobbyDetailsHandleByUiEventId(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Lobby_GetConnectString(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Lobby_ParseConnectString(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_Lobby_GetRTCRoomName(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Lobby_IsRTCRoomConnected(void *h, const void *o, int *c){ (void)h;(void)o; if(c)*c=0; return EOS_Success; }
uint64_t EOS_Lobby_AddNotifyLobbyUpdateReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLobbyMemberUpdateReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLobbyMemberStatusReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLobbyInviteReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLobbyInviteAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLobbyInviteRejected(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyJoinLobbyAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyLeaveLobbyRequested(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifyRTCRoomConnectionChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Lobby_AddNotifySendLobbyNativeInviteRequested(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Lobby_RemoveNotifyLobbyUpdateReceived(void *h, uint64_t id)            { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLobbyMemberUpdateReceived(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLobbyMemberStatusReceived(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLobbyInviteReceived(void *h, uint64_t id)            { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLobbyInviteAccepted(void *h, uint64_t id)            { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLobbyInviteRejected(void *h, uint64_t id)            { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyJoinLobbyAccepted(void *h, uint64_t id)              { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyLeaveLobbyRequested(void *h, uint64_t id)            { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifyRTCRoomConnectionChanged(void *h, uint64_t id)       { (void)h;(void)id; }
void EOS_Lobby_RemoveNotifySendLobbyNativeInviteRequested(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_LobbyModification_Release(void *m)                    { (void)m; }
int  EOS_LobbyModification_SetBucketId(void *m, const void *o) { (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_SetMaxMembers(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_SetPermissionLevel(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_SetInvitesAllowed(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_AddAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_RemoveAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_AddMemberAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_RemoveMemberAttribute(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_LobbyModification_SetAllowedPlatformIds(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
void EOS_LobbyDetails_Release(void *h)                         { (void)h; }
void EOS_LobbyDetails_Info_Release(void *i)                    { (void)i; }
void EOS_LobbyDetails_MemberInfo_Release(void *i)              { (void)i; }
void EOS_Lobby_Attribute_Release(void *a)                      { (void)a; }
void *EOS_LobbyDetails_GetLobbyOwner(void *h, const void *o)   { (void)h;(void)o; return NULL; }
int  EOS_LobbyDetails_CopyInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_LobbyDetails_GetAttributeCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_LobbyDetails_CopyAttributeByIndex(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_LobbyDetails_CopyAttributeByKey(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_LobbyDetails_GetMemberCount(void *h, const void *o)   { (void)h;(void)o; return 0; }
void *EOS_LobbyDetails_GetMemberByIndex(void *h, const void *o){ (void)h;(void)o; return NULL; }
int  EOS_LobbyDetails_GetMemberAttributeCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_LobbyDetails_CopyMemberAttributeByIndex(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_LobbyDetails_CopyMemberAttributeByKey(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_LobbyDetails_CopyMemberInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
void EOS_LobbySearch_Release(void *h)                          { (void)h; }
void EOS_LobbySearch_Find(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_LobbySearch_GetSearchResultCount(void *h)             { (void)h; return 0; }
int  EOS_LobbySearch_CopySearchResultByIndex(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_LobbySearch_SetLobbyId(void *h, const void *o)        { (void)h;(void)o; return EOS_Success; }
int  EOS_LobbySearch_SetTargetUserId(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }
int  EOS_LobbySearch_SetParameter(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_LobbySearch_RemoveParameter(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }
int  EOS_LobbySearch_SetMaxResults(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }

/* -------------------------------------------------------------------------
 * P2P
 * -------------------------------------------------------------------------*/
int  EOS_P2P_SendPacket(void *h, const void *o)                { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_GetNextReceivedPacketSize(void *h, const void *o, uint32_t *s){ (void)h;(void)o; if(s)*s=0; return EOS_Success; }
int  EOS_P2P_ReceivePacket(void *h, const void *o, void *b, uint32_t *s, void *c, void *r){ (void)h;(void)o;(void)b;(void)s;(void)c;(void)r; return EOS_Success; }
int  EOS_P2P_AcceptConnection(void *h, const void *o)          { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_CloseConnection(void *h, const void *o)           { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_CloseConnections(void *h, const void *o)          { (void)h;(void)o; return EOS_Success; }
void EOS_P2P_QueryNATType(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_P2P_GetNATType(void *h, const void *o, void *t)       { (void)h;(void)o;(void)t; return EOS_Success; }
int  EOS_P2P_SetRelayControl(void *h, const void *o)           { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_GetRelayControl(void *h, const void *o, void *c)  { (void)h;(void)o;(void)c; return EOS_Success; }
int  EOS_P2P_SetPortRange(void *h, const void *o)              { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_GetPortRange(void *h, const void *o, void *p, void *a){ (void)h;(void)o;(void)p;(void)a; return EOS_Success; }
int  EOS_P2P_SetPacketQueueSize(void *h, const void *o)        { (void)h;(void)o; return EOS_Success; }
int  EOS_P2P_GetPacketQueueInfo(void *h, const void *o, void *i){ (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_P2P_ClearPacketQueue(void *h, const void *o)          { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_P2P_AddNotifyPeerConnectionRequest(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_P2P_AddNotifyPeerConnectionEstablished(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_P2P_AddNotifyPeerConnectionInterrupted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_P2P_AddNotifyPeerConnectionClosed(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_P2P_AddNotifyIncomingPacketQueueFull(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_P2P_RemoveNotifyPeerConnectionRequest(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_P2P_RemoveNotifyPeerConnectionEstablished(void *h, uint64_t id)  { (void)h;(void)id; }
void EOS_P2P_RemoveNotifyPeerConnectionInterrupted(void *h, uint64_t id)  { (void)h;(void)id; }
void EOS_P2P_RemoveNotifyPeerConnectionClosed(void *h, uint64_t id)       { (void)h;(void)id; }
void EOS_P2P_RemoveNotifyIncomingPacketQueueFull(void *h, uint64_t id)    { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * Achievements
 * -------------------------------------------------------------------------*/
void EOS_Achievements_QueryDefinitions(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Achievements_QueryPlayerAchievements(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Achievements_UnlockAchievements(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Achievements_GetAchievementDefinitionCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Achievements_CopyAchievementDefinitionByIndex(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Achievements_CopyAchievementDefinitionByAchievementId(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Achievements_CopyAchievementDefinitionV2ByIndex(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Achievements_CopyAchievementDefinitionV2ByAchievementId(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Achievements_GetPlayerAchievementCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Achievements_CopyPlayerAchievementByIndex(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_Achievements_CopyPlayerAchievementByAchievementId(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_Achievements_GetUnlockedAchievementCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Achievements_CopyUnlockedAchievementByIndex(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
int  EOS_Achievements_CopyUnlockedAchievementByAchievementId(void *h, const void *o, void **a){ (void)h;(void)o; if(a)*a=NULL; return EOS_Success; }
void EOS_Achievements_Definition_Release(void *d)              { (void)d; }
void EOS_Achievements_DefinitionV2_Release(void *d)            { (void)d; }
void EOS_Achievements_PlayerAchievement_Release(void *a)       { (void)a; }
void EOS_Achievements_UnlockedAchievement_Release(void *a)     { (void)a; }
uint64_t EOS_Achievements_AddNotifyAchievementsUnlocked(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Achievements_AddNotifyAchievementsUnlockedV2(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Achievements_RemoveNotifyAchievementsUnlocked(void *h, uint64_t id) { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * Stats / Leaderboards
 * -------------------------------------------------------------------------*/
void EOS_Stats_IngestStat(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Stats_QueryStats(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Stats_GetStatsCount(void *h, const void *o)           { (void)h;(void)o; return 0; }
int  EOS_Stats_CopyStatByIndex(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_Stats_CopyStatByName(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
void EOS_Stats_Stat_Release(void *s)                           { (void)s; }
void EOS_Leaderboards_QueryLeaderboardDefinitions(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Leaderboards_QueryLeaderboardRanks(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Leaderboards_QueryLeaderboardUserScores(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Leaderboards_GetLeaderboardDefinitionCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Leaderboards_CopyLeaderboardDefinitionByIndex(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Leaderboards_CopyLeaderboardDefinitionByLeaderboardId(void *h, const void *o, void **d){ (void)h;(void)o; if(d)*d=NULL; return EOS_Success; }
int  EOS_Leaderboards_GetLeaderboardRecordCount(void *h)       { (void)h; return 0; }
int  EOS_Leaderboards_CopyLeaderboardRecordByIndex(void *h, const void *o, void **r){ (void)h;(void)o; if(r)*r=NULL; return EOS_Success; }
int  EOS_Leaderboards_CopyLeaderboardRecordByUserId(void *h, const void *o, void **r){ (void)h;(void)o; if(r)*r=NULL; return EOS_Success; }
int  EOS_Leaderboards_GetLeaderboardUserScoreCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Leaderboards_CopyLeaderboardUserScoreByIndex(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
int  EOS_Leaderboards_CopyLeaderboardUserScoreByUserId(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
void EOS_Leaderboards_Definition_Release(void *d)              { (void)d; }
void EOS_Leaderboards_LeaderboardDefinition_Release(void *d)   { (void)d; }
void EOS_Leaderboards_LeaderboardRecord_Release(void *r)       { (void)r; }
void EOS_Leaderboards_LeaderboardUserScore_Release(void *s)    { (void)s; }

/* -------------------------------------------------------------------------
 * UserInfo / Friends / Presence
 * -------------------------------------------------------------------------*/
void EOS_UserInfo_QueryUserInfo(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_UserInfo_QueryUserInfoByDisplayName(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_UserInfo_QueryUserInfoByExternalAccount(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_UserInfo_CopyUserInfo(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_UserInfo_CopyBestDisplayName(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_UserInfo_CopyBestDisplayNameWithPlatform(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_UserInfo_GetExternalUserInfoCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_UserInfo_CopyExternalUserInfoByIndex(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_UserInfo_CopyExternalUserInfoByAccountType(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_UserInfo_CopyExternalUserInfoByAccountId(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_UserInfo_GetLocalPlatformType(void *h)                { (void)h; return 0; }
void EOS_UserInfo_Release(void *i)                             { (void)i; }
void EOS_UserInfo_ExternalUserInfo_Release(void *e)            { (void)e; }
void EOS_UserInfo_BestDisplayName_Release(void *n)             { (void)n; }
void EOS_Friends_QueryFriends(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Friends_SendInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Friends_AcceptInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Friends_RejectInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Friends_GetFriendsCount(void *h, const void *o)       { (void)h;(void)o; return 0; }
void *EOS_Friends_GetFriendAtIndex(void *h, const void *o)     { (void)h;(void)o; return NULL; }
int  EOS_Friends_GetStatus(void *h, const void *o)             { (void)h;(void)o; return 0; }
int  EOS_Friends_GetBlockedUsersCount(void *h, const void *o)  { (void)h;(void)o; return 0; }
void *EOS_Friends_GetBlockedUserAtIndex(void *h, const void *o){ (void)h;(void)o; return NULL; }
uint64_t EOS_Friends_AddNotifyFriendsUpdate(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Friends_AddNotifyBlockedUsersUpdate(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Friends_RemoveNotifyFriendsUpdate(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_Friends_RemoveNotifyBlockedUsersUpdate(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_Presence_QueryPresence(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Presence_SetPresence(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Presence_HasPresence(void *h, const void *o)          { (void)h;(void)o; return 0; }
int  EOS_Presence_CopyPresence(void *h, const void *o, void **p){ (void)h;(void)o; if(p)*p=NULL; return EOS_Success; }
int  EOS_Presence_GetJoinInfo(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_Presence_CreatePresenceModification(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=_fake; return EOS_Success; }
void EOS_Presence_Info_Release(void *i)                        { (void)i; }
uint64_t EOS_Presence_AddNotifyOnPresenceChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_Presence_AddNotifyJoinGameAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Presence_RemoveNotifyOnPresenceChanged(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_Presence_RemoveNotifyJoinGameAccepted(void *h, uint64_t id)  { (void)h;(void)id; }
void EOS_PresenceModification_Release(void *m)                 { (void)m; }
int  EOS_PresenceModification_SetStatus(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_PresenceModification_SetRawRichText(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_PresenceModification_SetData(void *m, const void *o)  { (void)m;(void)o; return EOS_Success; }
int  EOS_PresenceModification_DeleteData(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }
int  EOS_PresenceModification_SetJoinInfo(void *m, const void *o){ (void)m;(void)o; return EOS_Success; }

/* -------------------------------------------------------------------------
 * Metrics / Reports / Sanctions
 * -------------------------------------------------------------------------*/
void EOS_Metrics_BeginPlayerSession(void *h, const void *o) { (void)h;(void)o; }
void EOS_Metrics_EndPlayerSession(void *h, const void *o)   { (void)h;(void)o; }
void EOS_Reports_SendPlayerBehaviorReport(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sanctions_QueryActivePlayerSanctions(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Sanctions_CreatePlayerSanctionAppeal(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Sanctions_GetPlayerSanctionCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Sanctions_CopyPlayerSanctionByIndex(void *h, const void *o, void **s){ (void)h;(void)o; if(s)*s=NULL; return EOS_Success; }
void EOS_Sanctions_PlayerSanction_Release(void *s)             { (void)s; }

/* -------------------------------------------------------------------------
 * AntiCheat (EAC disabled, but symbols still need to exist)
 * -------------------------------------------------------------------------*/
void EOS_AntiCheatServer_BeginSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_AntiCheatServer_EndSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_AntiCheatServer_RegisterClient(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_UnregisterClient(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_RegisterEvent(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogEvent(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogGameRoundStart(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogGameRoundEnd(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerSpawn(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerDespawn(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerRevive(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerTick(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerUseAbility(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerUseWeapon(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_LogPlayerTakeDamage(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_SetClientDetails(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_SetClientNetworkState(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_SetGameSessionId(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_ProtectMessage(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_AntiCheatServer_UnprotectMessage(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_AntiCheatServer_ReceiveMessageFromClient(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatServer_GetProtectMessageOutputLength(void *h, const void *o, uint32_t *s){ (void)h;(void)o; if(s)*s=0; return EOS_Success; }
uint64_t EOS_AntiCheatServer_AddNotifyClientActionRequired(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatServer_AddNotifyClientAuthStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatServer_AddNotifyMessageToClient(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_AntiCheatServer_RemoveNotifyClientActionRequired(void *h, uint64_t id)    { (void)h;(void)id; }
void EOS_AntiCheatServer_RemoveNotifyClientAuthStatusChanged(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_AntiCheatServer_RemoveNotifyMessageToClient(void *h, uint64_t id)         { (void)h;(void)id; }
void EOS_AntiCheatClient_BeginSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_AntiCheatClient_EndSession(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_AntiCheatClient_AddExternalIntegrityCatalog(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_PollStatus(void *h, const void *o, void *s){ (void)h;(void)o;(void)s; return EOS_Success; }
int  EOS_AntiCheatClient_ProtectMessage(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_AntiCheatClient_UnprotectMessage(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
int  EOS_AntiCheatClient_ReceiveMessageFromServer(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_ReceiveMessageFromPeer(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_SendMessageToServer(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_SendMessageToPeer(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_RegisterPeer(void *h, const void *o)  { (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_UnregisterPeer(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_AntiCheatClient_GetProtectMessageOutputLength(void *h, const void *o, uint32_t *s){ (void)h;(void)o; if(s)*s=0; return EOS_Success; }
int  EOS_AntiCheatClient_Reserved01(void *h, const void *o)    { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_AntiCheatClient_AddNotifyMessageToServer(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatClient_AddNotifyMessageToPeer(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatClient_AddNotifyPeerActionRequired(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatClient_AddNotifyPeerAuthStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_AntiCheatClient_AddNotifyClientIntegrityViolated(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_AntiCheatClient_RemoveNotifyMessageToServer(void *h, uint64_t id)         { (void)h;(void)id; }
void EOS_AntiCheatClient_RemoveNotifyMessageToPeer(void *h, uint64_t id)           { (void)h;(void)id; }
void EOS_AntiCheatClient_RemoveNotifyPeerActionRequired(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_AntiCheatClient_RemoveNotifyPeerAuthStatusChanged(void *h, uint64_t id)   { (void)h;(void)id; }
void EOS_AntiCheatClient_RemoveNotifyClientIntegrityViolated(void *h, uint64_t id) { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * UI
 * -------------------------------------------------------------------------*/
int  EOS_UI_ShowFriends(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); return EOS_Success; }
int  EOS_UI_HideFriends(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); return EOS_Success; }
int  EOS_UI_ShowBlockPlayer(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); return EOS_Success; }
int  EOS_UI_ShowReportPlayer(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); return EOS_Success; }
int  EOS_UI_ShowNativeProfile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); return EOS_Success; }
int  EOS_UI_GetFriendsVisible(void *h, const void *o)          { (void)h;(void)o; return 0; }
int  EOS_UI_GetFriendsExclusiveInput(void *h, const void *o)   { (void)h;(void)o; return 0; }
int  EOS_UI_GetToggleFriendsKey(void *h, const void *o, void *k){ (void)h;(void)o;(void)k; return EOS_Success; }
int  EOS_UI_SetToggleFriendsKey(void *h, const void *o)        { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_GetToggleFriendsButton(void *h, const void *o, void *b){ (void)h;(void)o;(void)b; return EOS_Success; }
int  EOS_UI_SetToggleFriendsButton(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_IsValidKeyCombination(void *h, int k)              { (void)h;(void)k; return 0; }
int  EOS_UI_IsValidButtonCombination(void *h, int b)           { (void)h;(void)b; return 0; }
int  EOS_UI_SetDisplayPreference(void *h, const void *o)       { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_GetNotificationLocationPreference(void *h)         { (void)h; return 0; }
int  EOS_UI_AcknowledgeEventId(void *h, const void *o)         { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_ReportInputState(void *h, const void *o)           { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_PrePresent(void *h, const void *o)                 { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_PauseSocialOverlay(void *h, const void *o)         { (void)h;(void)o; return EOS_Success; }
int  EOS_UI_IsSocialOverlayPaused(void *h)                     { (void)h; return 0; }
uint64_t EOS_UI_AddNotifyDisplaySettingsUpdated(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_UI_AddNotifyMemoryMonitor(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_UI_RemoveNotifyDisplaySettingsUpdated(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_UI_RemoveNotifyMemoryMonitor(void *h, uint64_t id)          { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * Ecom / Storage / Mods / KWS / Progression / CustomInvites / IntegratedPlatform
 * -------------------------------------------------------------------------*/
void EOS_Ecom_QueryOwnership(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_QueryOwnershipBySandboxIds(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_QueryOwnershipToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_QueryEntitlements(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_QueryEntitlementToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_QueryOffers(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_Checkout(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Ecom_RedeemEntitlements(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Ecom_GetEntitlementsCount(void *h, const void *o)        { (void)h;(void)o; return 0; }
int  EOS_Ecom_GetEntitlementsByNameCount(void *h, const void *o)  { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyEntitlementByIndex(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_Ecom_CopyEntitlementByNameAndIndex(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_Ecom_CopyEntitlementById(void *h, const void *o, void **e){ (void)h;(void)o; if(e)*e=NULL; return EOS_Success; }
int  EOS_Ecom_GetOfferCount(void *h, const void *o)               { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyOfferByIndex(void *h, const void *o, void **r)  { (void)h;(void)o; if(r)*r=NULL; return EOS_Success; }
int  EOS_Ecom_CopyOfferById(void *h, const void *o, void **r)     { (void)h;(void)o; if(r)*r=NULL; return EOS_Success; }
int  EOS_Ecom_GetOfferItemCount(void *h, const void *o)           { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyOfferItemByIndex(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_Ecom_CopyItemById(void *h, const void *o, void **i)      { (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_Ecom_GetItemImageInfoCount(void *h, const void *o)       { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyItemImageInfoByIndex(void *h, const void *o, void **k){ (void)h;(void)o; if(k)*k=NULL; return EOS_Success; }
int  EOS_Ecom_GetItemReleaseCount(void *h, const void *o)         { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyItemReleaseByIndex(void *h, const void *o, void **r){ (void)h;(void)o; if(r)*r=NULL; return EOS_Success; }
int  EOS_Ecom_GetOfferImageInfoCount(void *h, const void *o)      { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyOfferImageInfoByIndex(void *h, const void *o, void **k){ (void)h;(void)o; if(k)*k=NULL; return EOS_Success; }
int  EOS_Ecom_GetTransactionCount(void *h, const void *o)         { (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyTransactionByIndex(void *h, const void *o, void **t){ (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
int  EOS_Ecom_CopyTransactionById(void *h, const void *o, void **t){ (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
int  EOS_Ecom_GetLastRedeemedEntitlementsCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_Ecom_CopyLastRedeemedEntitlementByIndex(void *h, const void *o, void *b, uint32_t *s){ (void)h;(void)o;(void)b;(void)s; return EOS_Success; }
void EOS_Ecom_Entitlement_Release(void *e)                         { (void)e; }
void EOS_Ecom_CatalogItem_Release(void *i)                         { (void)i; }
void EOS_Ecom_CatalogOffer_Release(void *o)                        { (void)o; }
void EOS_Ecom_CatalogRelease_Release(void *r)                      { (void)r; }
void EOS_Ecom_KeyImageInfo_Release(void *k)                        { (void)k; }
int  EOS_Ecom_Transaction_GetTransactionId(void *t, void *b, uint32_t *s){ (void)t;(void)b;(void)s; return EOS_Success; }
int  EOS_Ecom_Transaction_GetEntitlementsCount(void *t, const void *o){ (void)t;(void)o; return 0; }
int  EOS_Ecom_Transaction_CopyEntitlementByIndex(void *t, const void *o, void **e){ (void)t;(void)o; if(e)*e=NULL; return EOS_Success; }
void EOS_Ecom_Transaction_Release(void *t)                         { (void)t; }
void EOS_PlayerDataStorage_QueryFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_QueryFileList(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_ReadFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_WriteFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_DeleteFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_DeleteCache(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_PlayerDataStorage_DuplicateFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_PlayerDataStorage_GetFileMetadataCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_PlayerDataStorage_CopyFileMetadataAtIndex(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=NULL; return EOS_Success; }
int  EOS_PlayerDataStorage_CopyFileMetadataByFilename(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=NULL; return EOS_Success; }
void EOS_PlayerDataStorage_FileMetadata_Release(void *m)           { (void)m; }
void EOS_PlayerDataStorageFileTransferRequest_Release(void *r)     { (void)r; }
int  EOS_PlayerDataStorageFileTransferRequest_GetFileRequestState(void *r){ (void)r; return 0; }
int  EOS_PlayerDataStorageFileTransferRequest_GetFilename(void *r, void *b, uint32_t *s){ (void)r;(void)b;(void)s; return EOS_Success; }
int  EOS_PlayerDataStorageFileTransferRequest_CancelRequest(void *r){ (void)r; return EOS_Success; }
void EOS_TitleStorage_QueryFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_TitleStorage_QueryFileList(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_TitleStorage_ReadFile(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_TitleStorage_DeleteCache(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_TitleStorage_GetFileMetadataCount(void *h, const void *o){ (void)h;(void)o; return 0; }
int  EOS_TitleStorage_CopyFileMetadataAtIndex(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=NULL; return EOS_Success; }
int  EOS_TitleStorage_CopyFileMetadataByFilename(void *h, const void *o, void **m){ (void)h;(void)o; if(m)*m=NULL; return EOS_Success; }
void EOS_TitleStorage_FileMetadata_Release(void *m)                { (void)m; }
void EOS_TitleStorageFileTransferRequest_Release(void *r)          { (void)r; }
int  EOS_TitleStorageFileTransferRequest_GetFileRequestState(void *r){ (void)r; return 0; }
int  EOS_TitleStorageFileTransferRequest_GetFilename(void *r, void *b, uint32_t *s){ (void)r;(void)b;(void)s; return EOS_Success; }
int  EOS_TitleStorageFileTransferRequest_CancelRequest(void *r)    { (void)r; return EOS_Success; }
void EOS_Mods_EnumerateMods(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Mods_InstallMod(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Mods_UninstallMod(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Mods_UpdateMod(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_Mods_CopyModInfo(void *h, const void *o, void **i)        { (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
void EOS_Mods_ModInfo_Release(void *i)                             { (void)i; }
void EOS_KWS_QueryAgeGate(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_KWS_CreateUser(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_KWS_QueryPermissions(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_KWS_RequestPermissions(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_KWS_UpdateParentEmail(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_KWS_GetPermissionsCount(void *h, const void *o)           { (void)h;(void)o; return 0; }
int  EOS_KWS_CopyPermissionByIndex(void *h, const void *o, void **p){ (void)h;(void)o; if(p)*p=NULL; return EOS_Success; }
int  EOS_KWS_GetPermissionByKey(void *h, const void *o, void *v)   { (void)h;(void)o;(void)v; return EOS_Success; }
void EOS_KWS_PermissionStatus_Release(void *p)                     { (void)p; }
uint64_t EOS_KWS_AddNotifyPermissionsUpdateReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_KWS_RemoveNotifyPermissionsUpdateReceived(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_ProgressionSnapshot_BeginSnapshot(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_ProgressionSnapshot_AddProgression(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
void EOS_ProgressionSnapshot_SubmitSnapshot(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_ProgressionSnapshot_DeleteSnapshot(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_ProgressionSnapshot_EndSnapshot(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }
void EOS_CustomInvites_SendCustomInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_CustomInvites_SendRequestToJoin(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_CustomInvites_AcceptRequestToJoin(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_CustomInvites_RejectRequestToJoin(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_CustomInvites_FinalizeInvite(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_CustomInvites_SetCustomInvite(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_CustomInvites_AddNotifyCustomInviteReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyCustomInviteAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyCustomInviteRejected(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyRequestToJoinReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyRequestToJoinAccepted(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyRequestToJoinRejected(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifyRequestToJoinResponseReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_CustomInvites_AddNotifySendCustomNativeInviteRequested(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_CustomInvites_RemoveNotifyCustomInviteReceived(void *h, uint64_t id)          { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyCustomInviteAccepted(void *h, uint64_t id)          { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyCustomInviteRejected(void *h, uint64_t id)          { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyRequestToJoinReceived(void *h, uint64_t id)         { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyRequestToJoinAccepted(void *h, uint64_t id)         { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyRequestToJoinRejected(void *h, uint64_t id)         { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifyRequestToJoinResponseReceived(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_CustomInvites_RemoveNotifySendCustomNativeInviteRequested(void *h, uint64_t id){ (void)h;(void)id; }
int  EOS_IntegratedPlatform_CreateIntegratedPlatformOptionsContainer(void *h, const void *o, void **c){ (void)h;(void)o; if(c)*c=_fake; return EOS_Success; }
void EOS_IntegratedPlatformOptionsContainer_Release(void *c)       { (void)c; }
int  EOS_IntegratedPlatformOptionsContainer_Add(void *c, const void *o){ (void)c;(void)o; return EOS_Success; }
int  EOS_IntegratedPlatform_SetUserLoginStatus(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
void EOS_IntegratedPlatform_FinalizeDeferredUserLogout(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_IntegratedPlatform_SetUserPreLogoutCallback(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_IntegratedPlatform_ClearUserPreLogoutCallback(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
uint64_t EOS_IntegratedPlatform_AddNotifyUserLoginStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_IntegratedPlatform_RemoveNotifyUserLoginStatusChanged(void *h, uint64_t id)  { (void)h;(void)id; }

/* -------------------------------------------------------------------------
 * RTC / RTCAudio / RTCData / RTCAdmin
 * -------------------------------------------------------------------------*/
void EOS_RTC_JoinRoom(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTC_LeaveRoom(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTC_BlockParticipant(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_RTC_SetRoomSetting(void *h, const void *o)                { (void)h;(void)o; return EOS_Success; }
int  EOS_RTC_SetSetting(void *h, const void *o)                    { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_RTC_AddNotifyDisconnected(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTC_AddNotifyParticipantStatusChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTC_AddNotifyRoomStatisticsUpdated(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_RTC_RemoveNotifyDisconnected(void *h, uint64_t id)             { (void)h;(void)id; }
void EOS_RTC_RemoveNotifyParticipantStatusChanged(void *h, uint64_t id) { (void)h;(void)id; }
void EOS_RTC_RemoveNotifyRoomStatisticsUpdated(void *h, uint64_t id)    { (void)h;(void)id; }

/* RTCAudio */
void EOS_RTCAudio_QueryInputDevicesInformation(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_QueryOutputDevicesInformation(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_UpdateSending(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_UpdateReceiving(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_UpdateSendingVolume(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_UpdateReceivingVolume(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAudio_UpdateParticipantVolume(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_RTCAudio_GetAudioInputDevicesCount(void *h, const void *o)  { (void)h;(void)o; return 0; }
int  EOS_RTCAudio_GetAudioOutputDevicesCount(void *h, const void *o) { (void)h;(void)o; return 0; }
int  EOS_RTCAudio_GetInputDevicesCount(void *h)                       { (void)h; return 0; }
int  EOS_RTCAudio_GetOutputDevicesCount(void *h)                      { (void)h; return 0; }
void *EOS_RTCAudio_GetAudioInputDeviceByIndex(void *h, const void *o) { (void)h;(void)o; return NULL; }
void *EOS_RTCAudio_GetAudioOutputDeviceByIndex(void *h, const void *o){ (void)h;(void)o; return NULL; }
int  EOS_RTCAudio_CopyInputDeviceInformationByIndex(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
int  EOS_RTCAudio_CopyOutputDeviceInformationByIndex(void *h, const void *o, void **i){ (void)h;(void)o; if(i)*i=NULL; return EOS_Success; }
void EOS_RTCAudio_InputDeviceInformation_Release(void *i)  { (void)i; }
void EOS_RTCAudio_OutputDeviceInformation_Release(void *i) { (void)i; }
int  EOS_RTCAudio_SetAudioInputSettings(void *h, const void *o)       { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_SetAudioOutputSettings(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_SetInputDeviceSettings(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_SetOutputDeviceSettings(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_SendAudio(void *h, const void *o)                   { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_RegisterPlatformUser(void *h, const void *o)        { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_UnregisterPlatformUser(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_RegisterPlatformAudioUser(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }
int  EOS_RTCAudio_UnregisterPlatformAudioUser(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_RTCAudio_AddNotifyAudioDevicesChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCAudio_AddNotifyAudioInputState(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCAudio_AddNotifyAudioOutputState(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCAudio_AddNotifyAudioBeforeSend(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCAudio_AddNotifyAudioBeforeRender(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCAudio_AddNotifyParticipantUpdated(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_RTCAudio_RemoveNotifyAudioDevicesChanged(void *h, uint64_t id)  { (void)h;(void)id; }
void EOS_RTCAudio_RemoveNotifyAudioInputState(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_RTCAudio_RemoveNotifyAudioOutputState(void *h, uint64_t id)     { (void)h;(void)id; }
void EOS_RTCAudio_RemoveNotifyAudioBeforeSend(void *h, uint64_t id)      { (void)h;(void)id; }
void EOS_RTCAudio_RemoveNotifyAudioBeforeRender(void *h, uint64_t id)    { (void)h;(void)id; }
void EOS_RTCAudio_RemoveNotifyParticipantUpdated(void *h, uint64_t id)   { (void)h;(void)id; }

/* RTCData */
void EOS_RTCData_UpdateSending(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCData_UpdateReceiving(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_RTCData_SendData(void *h, const void *o)                     { (void)h;(void)o; return EOS_Success; }
uint64_t EOS_RTCData_AddNotifyDataReceived(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
uint64_t EOS_RTCData_AddNotifyParticipantUpdated(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_RTCData_RemoveNotifyDataReceived(void *h, uint64_t id)       { (void)h;(void)id; }
void EOS_RTCData_RemoveNotifyParticipantUpdated(void *h, uint64_t id) { (void)h;(void)id; }

/* RTCAdmin */
void EOS_RTCAdmin_QueryJoinRoomToken(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAdmin_Kick(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_RTCAdmin_SetParticipantHardMute(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
int  EOS_RTCAdmin_CopyUserTokenByIndex(void *h, const void *o, void **t){ (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
int  EOS_RTCAdmin_CopyUserTokenByUserId(void *h, const void *o, void **t){ (void)h;(void)o; if(t)*t=NULL; return EOS_Success; }
void EOS_RTCAdmin_UserToken_Release(void *t)                           { (void)t; }

/* Audio (non-RTC) */
int  EOS_Audio_CreateNewInputStream(void *h, const void *o)            { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_CreateNewOutputStream(void *h, const void *o)           { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_DestroyInputStream(void *h, const void *o)              { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_DestroyOutputStream(void *h, const void *o)             { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_StartInputStream(void *h, const void *o)                { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_StopInputStream(void *h, const void *o)                 { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_StartOutputStream(void *h, const void *o)               { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_StopOutputStream(void *h, const void *o)                { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_GetInputStreamInfo(void *h, const void *o, void *i)     { (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_Audio_GetOutputStreamInfo(void *h, const void *o, void *i)    { (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_Audio_GetInputDeviceInfo(void *h, const void *o, void *i)     { (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_Audio_GetOutputDeviceInfo(void *h, const void *o, void *i)    { (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_Audio_IsInputStreamSilent(void *h, const void *o, int *s)     { (void)h;(void)o; if(s)*s=1; return EOS_Success; }
int  EOS_Audio_IsInputStreamDeviceDisconnected(void *h, const void *o, int *d){ (void)h;(void)o; if(d)*d=1; return EOS_Success; }
int  EOS_Audio_SetFeatureEnabledForInputStream(void *h, const void *o) { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_EnableCommunicationsModeOutputDevices(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_RegisterUser(void *h, const void *o)                    { (void)h;(void)o; return EOS_Success; }
int  EOS_Audio_UnregisterUser(void *h, const void *o)                  { (void)h;(void)o; return EOS_Success; }
void EOS_Audio_QueryInputDevices(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
void EOS_Audio_QueryOutputDevices(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o; _fire(cb, cd, EOS_Success); }
uint64_t EOS_Audio_SetNotifyDevicesChanged(void *h, const void *o, void *cd, eos_cb cb)
    { (void)h;(void)o;(void)cd;(void)cb; return 1; }
void EOS_Audio_RemoveNotifyDevicesChanged(void *h, uint64_t id) { (void)h;(void)id; }

/* BroadcastAudio */
int  EOS_BroadcastAudio_CreateNewInputStream(void *h, const void *o)   { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_CreateNewOutputStream(void *h, const void *o)  { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_DestroyInputStream(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_DestroyOutputStream(void *h, const void *o)    { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_StartInputStream(void *h, const void *o)       { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_StopInputStream(void *h, const void *o)        { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_StartOutputStream(void *h, const void *o)      { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_StopOutputStream(void *h, const void *o)       { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_GetInputStreamInfo(void *h, const void *o, void *i){ (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_BroadcastAudio_GetOutputStreamInfo(void *h, const void *o, void *i){ (void)h;(void)o;(void)i; return EOS_Success; }
int  EOS_BroadcastAudio_GetCurrentMicAmplitude(void *h, const void *o, float *a){ (void)h;(void)o; if(a)*a=0.f; return EOS_Success; }
int  EOS_BroadcastAudio_GetCurrentGainLevel(void *h, const void *o, float *g){ (void)h;(void)o; if(g)*g=0.f; return EOS_Success; }
int  EOS_BroadcastAudio_SetMicProcessingSettings(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_SetEncoderSettings(void *h, const void *o)     { (void)h;(void)o; return EOS_Success; }
int  EOS_BroadcastAudio_PushPacketToOutputStream(void *h, const void *o){ (void)h;(void)o; return EOS_Success; }
