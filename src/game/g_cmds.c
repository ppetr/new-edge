/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Tremulous.

Tremulous is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Tremulous is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
/*
===========================================================================
TREMULOUS EDGE MOD SRC FILE
===========================================================================
*/
#include "g_local.h"

qboolean G_WeaponIsAllowed( weapon_t weapon ) {
  if (!BG_WeaponIsAllowed( weapon )) return qfalse;
  if (g_minTeamSizePerBuilderH.integer > 0) {
    if (weapon == WP_HBUILD) {
      CalculateRanks();
      if (level.numHumanBuilders > level.numHumanClients / g_minTeamSizePerBuilderH.integer + 1)
       return qfalse;
    }
  }
  return qtrue;
}

qboolean G_ClassIsAllowed( class_t class ) {
  if (!BG_ClassIsAllowed( class )) return qfalse;
  if (g_minTeamSizePerBuilderA.integer > 0) {
    if (class == PCL_ALIEN_BUILDER0 || class == PCL_ALIEN_BUILDER0_UPG) {
      CalculateRanks();
      if (level.numAlienBuilders > level.numAlienClients / g_minTeamSizePerBuilderA.integer + 1)
       return qfalse;
    }
  }

  return qtrue;
}

/*
==================
G_SanitiseString

Remove color codes and non-alphanumeric characters from a string
==================
*/
void G_SanitiseString( char *in, char *out, int len )
{
  len--;

  while( *in && len > 0 )
  {
    if( Q_IsColorString( in ) )
    {
      in += 2;    // skip color code
      continue;
    }

    if( isalnum( *in ) )
    {
        *out++ = tolower( *in );
        len--;
    }
    in++;
  }
  *out = 0;
}

/*
==================
G_SingleLineString

Remove semicolons and newlines from a string
==================
*/
void G_SingleLineString( char *in, char *out, int len )
{
  len--;

  while( *in && len > 0 )
  {
    if( isprint( *in ) && *in != ';' )
    {
      *out++ = *in;
      len--;
    }
    in++;
  }
  *out = '\0';
}

/*
==================
G_ClientNumberFromString

Returns a player number for either a number or name string
Returns -1 and optionally sets err if invalid or not exactly 1 match
err will have a trailing \n if set
==================
*/
int G_ClientNumberFromString( char *s, char *err, int len )
{
  gclient_t *cl;
  int       i, found = 0, m = -1;
  char      s2[ MAX_NAME_LENGTH ];
  char      n2[ MAX_NAME_LENGTH ];
  char      *p = err;
  int       l, l2 = len;

  if( !s[ 0 ] )
  {
    if( p )
      Q_strncpyz( p, "no player name or slot # provided\n", len );

    return -1;
  }

  // numeric values are just slot numbers
  for( i = 0; s[ i ] && isdigit( s[ i ] ); i++ );
  if( !s[ i ] )
  {
    i = atoi( s );

    if( i < 0 || i >= level.maxclients )
      return -1;

    cl = &level.clients[ i ];

    if( cl->pers.connected == CON_DISCONNECTED )
    {
      if( p )
        Q_strncpyz( p, "no player connected in that slot #\n", len );

      return -1;
    }

    return i;
  }

  G_SanitiseString( s, s2, sizeof( s2 ) );
  if( !s2[ 0 ] )
  {
    if( p )
      Q_strncpyz( p, "no player name provided\n", len );

    return -1;
  }

  if( p )
  {
    Q_strncpyz( p, "more than one player name matches. "
                "be more specific or use the slot #:\n", l2 );
    l = strlen( p );
    p += l;
    l2 -= l;
  }

  // check for a name match
  for( i = 0, cl = level.clients; i < level.maxclients; i++, cl++ )
  {
    if( cl->pers.connected == CON_DISCONNECTED )
      continue;

    G_SanitiseString( cl->pers.netname, n2, sizeof( n2 ) );

    if( !strcmp( n2, s2 ) )
      return i;

    if( strstr( n2, s2 ) )
    {
      if( p )
      {
        l = Q_snprintf( p, l2, "%-2d - %s^7\n", i, cl->pers.netname );
        p += l;
        l2 -= l;
      }

      found++;
      m = i;
    }
  }

  if( found == 1 )
    return m;

  if( found == 0 && err )
    Q_strncpyz( err, "no connected player by that name or slot #\n", len );

  return -1;
}

/*
==================
G_ClientNumbersFromString

Sets plist to an array of integers that represent client numbers that have
names that are a partial match for s.

Returns number of matching clientids up to max.
==================
*/
int G_ClientNumbersFromString( char *s, int *plist, int max )
{
  gclient_t *p;
  int i, found = 0;
  char *endptr;
  char n2[ MAX_NAME_LENGTH ] = {""};
  char s2[ MAX_NAME_LENGTH ] = {""};

  if( max == 0 )
    return 0;

  if( !s[ 0 ] )
    return 0;

  // if a number is provided, it is a clientnum
  i = strtol( s, &endptr, 10 );
  if( *endptr == '\0' )
  {
    if( i >= 0 && i < level.maxclients )
    {
      p = &level.clients[ i ];
      if( p->pers.connected != CON_DISCONNECTED )
      {
        *plist = i;
        return 1;
      }
    }
    // we must assume that if only a number is provided, it is a clientNum
    return 0;
  }

  // now look for name matches
  G_SanitiseString( s, s2, sizeof( s2 ) );
  if( !s2[ 0 ] )
    return 0;
  for( i = 0; i < level.maxclients && found < max; i++ )
  {
    p = &level.clients[ i ];
    if( p->pers.connected == CON_DISCONNECTED )
    {
      continue;
    }
    G_SanitiseString( p->pers.netname, n2, sizeof( n2 ) );
    if( strstr( n2, s2 ) )
    {
      *plist++ = i;
      found++;
    }
  }
  return found;
}

/*
==================
ScoreboardMessage

==================
*/
void ScoreboardMessage( gentity_t *ent )
{
  char      entry[ 1024 ];
  char      string[ 1400 ];
  int       stringlength;
  int       i, j;
  gclient_t *cl;
  int       numSorted;
  weapon_t  weapon = WP_NONE;
  upgrade_t upgrade = UP_NONE;

  // send the latest information on all clients
  string[ 0 ] = 0;
  stringlength = 0;

  numSorted = level.numConnectedClients;

  for( i = 0; i < numSorted; i++ )
  {
    int   ping;

    cl = &level.clients[ level.sortedClients[ i ] ];

    if( cl->pers.connected == CON_CONNECTING )
      ping = -1;
    else
      ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

    if( cl->sess.spectatorState == SPECTATOR_NOT &&
        ( ent->client->pers.teamSelection == TEAM_NONE ||
          cl->pers.teamSelection == ent->client->pers.teamSelection ) )
    {
      weapon = cl->ps.weapon;

      if( BG_InventoryContainsUpgrade( UP_BATTLESUIT, cl->ps.stats ) )
        upgrade = UP_BATTLESUIT;
      else if( BG_InventoryContainsUpgrade( UP_JETPACK, cl->ps.stats ) )
        upgrade = UP_JETPACK;
      else if( BG_InventoryContainsUpgrade( UP_BATTPACK, cl->ps.stats ) )
        upgrade = UP_BATTPACK;
      else if( BG_InventoryContainsUpgrade( UP_HELMET, cl->ps.stats ) )
        upgrade = UP_HELMET;
      else if( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, cl->ps.stats ) )
        upgrade = UP_LIGHTARMOUR;
      else if( BG_InventoryContainsUpgrade( UP_BIOKIT, cl->ps.stats ) )
        upgrade = UP_BIOKIT;
      else if( BG_InventoryContainsUpgrade( UP_CLOAK, cl->ps.stats ) )
        upgrade = UP_CLOAK;
      else
        upgrade = UP_NONE;
    }
    else
    {
      weapon = WP_NONE;
      upgrade = UP_NONE;
    }

    Com_sprintf( entry, sizeof( entry ),
      " %d %d %d %d %d %d", level.sortedClients[ i ], cl->ps.persistant[ PERS_SCORE ],
      ping, ( level.time - cl->pers.enterTime ) / 60000, weapon, upgrade );

    j = strlen( entry );

    if( stringlength + j >= 1024 )
      break;

    strcpy( string + stringlength, entry );
    stringlength += j;
  }

  trap_SendServerCommand( ent-g_entities, va( "scores %i %i%s",
    level.alienKills, level.humanKills, string ) );
}


/*
==================
ConcatArgs
==================
*/
char *ConcatArgs( int start )
{
  int         i, c, tlen;
  static char line[ MAX_STRING_CHARS ];
  int         len;
  char        arg[ MAX_STRING_CHARS ];

  len = 0;
  c = trap_Argc( );

  for( i = start; i < c; i++ )
  {
    trap_Argv( i, arg, sizeof( arg ) );
    tlen = strlen( arg );

    if( len + tlen >= MAX_STRING_CHARS - 1 )
      break;

    memcpy( line + len, arg, tlen );
    len += tlen;

    if( len == MAX_STRING_CHARS - 1 )
      break;

    if( i != c - 1 )
    {
      line[ len ] = ' ';
      len++;
    }
  }

  line[ len ] = 0;

  return line;
}

/*
==================
ConcatArgsPrintable
Duplicate of concatargs but enquotes things that need to be
Used to log command arguments in a way that preserves user intended tokenizing
==================
*/
char *ConcatArgsPrintable( int start )
{
  int         i, c, tlen;
  static char line[ MAX_STRING_CHARS ];
  int         len;
  char        arg[ MAX_STRING_CHARS + 2 ];
  char        *printArg;

  len = 0;
  c = trap_Argc( );

  for( i = start; i < c; i++ )
  {
    printArg = arg;
    trap_Argv( i, arg, sizeof( arg ) );
    if( strchr( arg, ' ' ) )
      printArg = va( "\"%s\"", arg );
    tlen = strlen( printArg );

    if( len + tlen >= MAX_STRING_CHARS - 1 )
      break;

    memcpy( line + len, printArg, tlen );
    len += tlen;

    if( len == MAX_STRING_CHARS - 1 )
      break;

    if( i != c - 1 )
    {
      line[ len ] = ' ';
      len++;
    }
  }

  line[ len ] = 0;

  return line;
}


/*
==================
Cmd_Give_f
Give items to a client
==================
*/
void Cmd_Give_f( gentity_t *ent )
{
  char      *name;
  qboolean  give_all = qfalse;

  if( trap_Argc( ) < 2 )
  {
    ADMP( "usage: give [what]\n" );
    ADMP( "usage: valid choices are: all, health, funds [amount], stamina, "
          "poison, gas, ammo\n" );
    return;
  }

  name = ConcatArgs( 1 );
  if( Q_stricmp( name, "all" ) == 0 )
    give_all = qtrue;

  if( give_all || Q_stricmp( name, "health" ) == 0 )
  {
    ent->health = ent->client->ps.stats[ STAT_MAX_HEALTH ];
    BG_AddUpgradeToInventory( UP_MEDKIT, ent->client->ps.stats );
  }

  if( give_all || Q_stricmpn( name, "funds", 5 ) == 0 )
  {
    float credits;

    if( give_all || trap_Argc( ) < 3 )
      credits = 30000.0f;
    else
    {
      credits = atof( name + 6 ) *
        ( ent->client->pers.teamSelection == 
          TEAM_ALIENS ? ALIEN_CREDITS_PER_KILL : 1.0f );

      // clamp credits manually, as G_AddCreditToClient() expects a short int
      if( credits > SHRT_MAX )
        credits = 30000.0f;
      else if( credits < SHRT_MIN )
        credits = -30000.0f;
    }

    G_AddCreditToClient( ent->client, (short)credits, qtrue );
  }

  if( give_all || Q_stricmp( name, "stamina" ) == 0 )
    ent->client->ps.stats[ STAT_STAMINA ] = STAMINA_MAX;

  if( Q_stricmp( name, "poison" ) == 0 )
  {
    if( ent->client->pers.teamSelection == TEAM_HUMANS )
    {
      ent->client->ps.stats[ STAT_STATE ] |= SS_POISONED;
      ent->client->lastPoisonTime = level.time;
      ent->client->lastPoisonClient = ent;
    }
    else
    {
      ent->client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
      ent->client->boostedTime = level.time;
    }
  }

  if( Q_stricmp( name, "gas" ) == 0 )
  {
    ent->client->ps.eFlags |= EF_POISONCLOUDED;
    ent->client->lastPoisonCloudedTime = level.time;
    trap_SendServerCommand( ent->client->ps.clientNum,
                              "poisoncloud" );
  }

  if( give_all || Q_stricmp( name, "ammo" ) == 0 )
  {
    gclient_t *client = ent->client;

    if( client->ps.weapon != WP_ALEVEL3_UPG &&
        BG_Weapon( client->ps.weapon )->infiniteAmmo )
      return;

    client->ps.ammo = BG_Weapon( client->ps.weapon )->maxAmmo;
    client->ps.clips = BG_Weapon( client->ps.weapon )->maxClips;

    if( BG_Weapon( client->ps.weapon )->usesEnergy &&
        BG_InventoryContainsUpgrade( UP_BATTPACK, client->ps.stats ) )
      client->ps.ammo = (int)( (float)client->ps.ammo * BATTPACK_MODIFIER );
  }
}


/*
==================
Cmd_God_f

Sets client to godmode

argv(0) god
==================
*/
void Cmd_God_f( gentity_t *ent )
{
  char  *msg;

  ent->flags ^= FL_GODMODE;

  if( !( ent->flags & FL_GODMODE ) )
    msg = "godmode OFF\n";
  else
    msg = "godmode ON\n";

  trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Notarget_f

Sets client to notarget

argv(0) notarget
==================
*/
void Cmd_Notarget_f( gentity_t *ent )
{
  char  *msg;

  ent->flags ^= FL_NOTARGET;

  if( !( ent->flags & FL_NOTARGET ) )
    msg = "notarget OFF\n";
  else
    msg = "notarget ON\n";

  trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_Noclip_f

argv(0) noclip
==================
*/
void Cmd_Noclip_f( gentity_t *ent )
{
  char  *msg;

  if( ent->client->noclip )
    msg = "noclip OFF\n";
  else
    msg = "noclip ON\n";

  ent->client->noclip = !ent->client->noclip;

  trap_SendServerCommand( ent - g_entities, va( "print \"%s\"", msg ) );
}


/*
==================
Cmd_LevelShot_f

This is just to help generate the level pictures
for the menus.  It goes to the intermission immediately
and sends over a command to the client to resize the view,
hide the scoreboard, and take a special screenshot
==================
*/
void Cmd_LevelShot_f( gentity_t *ent )
{
  BeginIntermission( );
  trap_SendServerCommand( ent - g_entities, "clientLevelShot" );
}

/*
=================
Cmd_Kill_f
=================
*/
void Cmd_Kill_f( gentity_t *ent )
{
  if( g_cheats.integer )
  {
    ent->flags &= ~FL_GODMODE;
    ent->client->ps.stats[ STAT_HEALTH ] = ent->health = 0;
    player_die( ent, ent, ent, 100000, MOD_SUICIDE );
  }
  else
  {
    if( ent->suicideTime == 0 )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You will suicide in 20 seconds\n\"" );
      ent->suicideTime = level.time + 20000;
    }
    else if( ent->suicideTime > level.time )
    {
      trap_SendServerCommand( ent-g_entities, "print \"Suicide cancelled\n\"" );
      ent->suicideTime = 0;
    }
  }
}

/*
=================
Cmd_Team_f
=================
*/
void Cmd_Team_f( gentity_t *ent )
{
  team_t    team;
  team_t    oldteam = ent->client->pers.teamSelection;
  char      s[ MAX_TOKEN_CHARS ];
  qboolean  force = G_admin_permission( ent, ADMF_FORCETEAMCHANGE );
  int       aliens = level.numAlienClients;
  int       humans = level.numHumanClients;

  if( oldteam == TEAM_ALIENS )
    aliens--;
  else if( oldteam == TEAM_HUMANS )
    humans--;

  // stop team join spam
  if( ent->client->pers.teamChangeTime && 
      level.time - ent->client->pers.teamChangeTime < 1000 )
    return;

  // stop switching teams for gameplay exploit reasons by enforcing a long
  // wait before they can come back
  if( !force && !g_cheats.integer && ent->client->pers.aliveSeconds && 
      level.time - ent->client->pers.teamChangeTime < 30000 )
  {
    trap_SendServerCommand( ent-g_entities,
      va( "print \"You must wait another %d seconds before changing teams again\n\"",
        (int) ( ( 30000 - ( level.time - ent->client->pers.teamChangeTime ) ) / 1000.f ) ) );
    return;
  }

  // disallow joining teams during warmup
  if( g_doWarmup.integer && ( ( level.warmupTime - level.time ) / 1000 ) > 0 )
  {
    G_TriggerMenu( ent - g_entities, MN_WARMUP );
    return;
  }

  trap_Argv( 1, s, sizeof( s ) );

  if( !s[ 0 ] )
  {
    trap_SendServerCommand( ent-g_entities, va( "print \"team: %s\n\"",
      BG_TeamName( oldteam ) ) );
    return;
  }

  if( !Q_stricmp( s, "auto" ) )
  {
    if( level.humanTeamLocked && level.alienTeamLocked )
      team = TEAM_NONE;
    else if( level.humanTeamLocked || humans > aliens )
      team = TEAM_ALIENS;

    else if( level.alienTeamLocked || aliens > humans )
      team = TEAM_HUMANS;
    else
      team = TEAM_ALIENS + rand( ) / ( RAND_MAX / 2 + 1 );
  }
  else switch( G_TeamFromString( s ) )
  {
    case TEAM_NONE:
      team = TEAM_NONE;
      break;

    case TEAM_ALIENS:
      if( level.alienTeamLocked )
      {
        G_TriggerMenu( ent - g_entities, MN_A_TEAMLOCKED );
        return;
      }
      else if( level.humanTeamLocked )
        force = qtrue;

      if( !force && g_teamForceBalance.integer && aliens > humans )
      {
        G_TriggerMenu( ent - g_entities, MN_A_TEAMFULL );
        return;
      }

      team = TEAM_ALIENS;
      break;

    case TEAM_HUMANS:
      if( level.humanTeamLocked )
      {
        G_TriggerMenu( ent - g_entities, MN_H_TEAMLOCKED );
        return;
      }
      else if( level.alienTeamLocked )
        force = qtrue;

      if( !force && g_teamForceBalance.integer && humans > aliens )
      {
        G_TriggerMenu( ent - g_entities, MN_H_TEAMFULL );
        return;
      }

      team = TEAM_HUMANS;
      break;

    default:
      trap_SendServerCommand( ent-g_entities,
        va( "print \"Unknown team: %s\n\"", s ) );
      return;
  }

  // stop team join spam
  if( oldteam == team )
    return;

  if( team != TEAM_NONE && g_maxGameClients.integer &&
    level.numPlayingClients >= g_maxGameClients.integer )
  {
    G_TriggerMenu( ent - g_entities, MN_PLAYERLIMIT );
    return;
  }

  // Apply the change
  G_ChangeTeam( ent, team );
}

/*
==================
G_CensorString
==================
*/
static char censors[ 20000 ];
static int numcensors;

void G_LoadCensors( void )
{
  char *text_p, *token;
  char text[ 20000 ];
  char *term;
  int  len;
  fileHandle_t f;

  numcensors = 0;

  if( !g_censorship.string[ 0 ] )
    return;

  len = trap_FS_FOpenFile( g_censorship.string, &f, FS_READ );
  if( len < 0 )
  {
    Com_Printf( S_COLOR_RED "ERROR: Censors file %s doesn't exist\n",
      g_censorship.string );
    return;
  }
  if( len == 0 || len >= sizeof( text ) - 1 )
  {
    trap_FS_FCloseFile( f );
    Com_Printf( S_COLOR_RED "ERROR: Censors file %s is %s\n",
      g_censorship.string, len == 0 ? "empty" : "too long" );
    return;
  }
  trap_FS_Read( text, len, f );
  trap_FS_FCloseFile( f );
  text[ len ] = 0;

  term = censors;

  text_p = text;
  while( 1 )
  {
    token = COM_Parse( &text_p );
    if( !*token || sizeof( censors ) - ( term - censors ) < 4 )
      break;
    Q_strncpyz( term, token, sizeof( censors ) - ( term - censors ) );
    Q_strlwr( term );
    term += strlen( term ) + 1;
    if( sizeof( censors ) - ( term - censors ) == 0 )
      break;
    token = COM_ParseExt( &text_p, qfalse );
    Q_strncpyz( term, token, sizeof( censors ) - ( term - censors ) );
    term += strlen( term ) + 1;
    numcensors++;
  }
  G_Printf( "Parsed %d string replacements\n", numcensors );
}

void G_CensorString( char *out, const char *in, int len, gentity_t *ent )
{
  const char *s, *m;
  int  i;

  if( !numcensors || G_admin_permission( ent, ADMF_NOCENSORFLOOD) )
  {
    Q_strncpyz( out, in, len );
    return;
  }

  len--;
  while( *in )
  {
    if( Q_IsColorString( in ) )
    {
      if( len < 2 )
        break;
      *out++ = *in++;
      *out++ = *in++;
      len -= 2;
      continue;
    }
    if( !isalnum( *in ) )
    {
      if( len < 1 )
        break;
      *out++ = *in++;
      len--;
      continue;
    }
    m = censors;
    for( i = 0; i < numcensors; i++, m++ )
    {
      s = in;
      while( *s && *m )
      {
        if( Q_IsColorString( s ) )
        {
          s += 2;
          continue;
        }
        if( !isalnum( *s ) )
        {
          s++;
          continue;
        }
        if( tolower( *s ) != *m )
          break;
        s++;
        m++;
      }
      // match
      if( !*m )
      {
        in = s;
        m++;
        while( *m )
        {
          if( len < 1 )
            break;
          *out++ = *m++;
          len--;
        }
        break;
      }
      else
      {
        while( *m )
          m++;
        m++;
        while( *m )
          m++;
      }
    }
    if( len < 1 )
      break;
    // no match
    if( i == numcensors )
    {
      *out++ = *in++;
      len--;
    }
  }
  *out = 0;
}

/*
==================
G_Say
==================
*/
static qboolean G_SayTo( gentity_t *ent, gentity_t *other, saymode_t mode, const char *message )
{

  //int clientNum;

  if( !other )
    return qfalse;

  if( !other->inuse )
    return qfalse;

  if( !other->client )
    return qfalse;              

  if( other->client->pers.connected != CON_CONNECTED )
    return qfalse;

  // ignore messages from people in /ignore list
  if( Com_ClientListContains( &other->client->sess.ignoreList, (int)( ent - g_entities ) ) )
    return qfalse;
  
  if( ( ent && !OnSameTeam( ent, other ) ) &&
      ( mode == SAY_TEAM || mode == SAY_AREA || mode == SAY_TPRIVMSG ) )
  {
    if( other->client->pers.teamSelection != TEAM_NONE )
      return qfalse;

    // specs with ADMF_SPEC_ALLCHAT flag can see team chat
    if( !G_admin_permission( other, ADMF_SPEC_ALLCHAT ) && mode != SAY_TPRIVMSG )
      return qfalse;
  }

  trap_SendServerCommand( other-g_entities, va( "chat %d %d \"%s\"",
    ent ? (int)(ent-g_entities) : -1,
    mode,
    message ) );

  return qtrue;
}

void G_Say( gentity_t *ent, saymode_t mode, const char *chatText )
{
  int         j;
  gentity_t   *other;
  // don't let text be too long for malicious reasons
  char        text[ MAX_SAY_TEXT ];

  // check if blocked by g_specChat 0
  if( ( !g_specChat.integer ) && ( mode != SAY_TEAM ) &&
      ( ent ) && ( ent->client->pers.teamSelection == TEAM_NONE ) &&
      ( !G_admin_permission( ent, ADMF_NOCENSORFLOOD ) ) )
  {
    trap_SendServerCommand( ent-g_entities, "print \"say: Global chatting for "
      "spectators has been disabled. You may only use team chat.\n\"" );
    mode = SAY_TEAM;
  }

  switch( mode )
  {
    case SAY_ALL:
      G_LogPrintf( "Say: %d \"%s" S_COLOR_WHITE "\": " S_COLOR_GREEN "%s\n",
        ( ent ) ? ent - g_entities : -1,
        ( ent ) ? ent->client->pers.netname : "console", chatText );
      break;
    case SAY_TEAM:
      // console say_team is handled in g_svscmds, not here
      if( !ent || !ent->client )
        Com_Error( ERR_FATAL, "SAY_TEAM by non-client entity\n" );
      G_LogPrintf( "SayTeam: %d \"%s" S_COLOR_WHITE "\": " S_COLOR_CYAN "%s\n",
        ent - g_entities, ent->client->pers.netname, chatText );
      break;
    case SAY_RAW:
      if( ent )
        Com_Error( ERR_FATAL, "SAY_RAW by client entity\n" );
      G_LogPrintf( "Chat: -1 \"console\": %s\n", chatText );
    default:
      break;
  }

  G_CensorString( text, chatText, sizeof( text ), ent );

  // send it to all the apropriate clients
  for( j = 0; j < level.maxclients; j++ )
  {
    other = &g_entities[ j ];
    G_SayTo( ent, other, mode, text );
  }
}
/*
==================
Cmd_Where_f
==================
*/
void Cmd_Where_f( gentity_t *ent )
{
  if( !ent->client )
    return;
  trap_SendServerCommand( ent - g_entities,
                          va( "print \"origin: %f %f %f\n\"",
                              ent->s.origin[ 0 ], ent->s.origin[ 1 ],
                              ent->s.origin[ 2 ] ) );
}

/*
==================
Cmd_CallVote_f
==================
*/
void Cmd_CallVote_f( gentity_t *ent )
{
  char   cmd[ MAX_TOKEN_CHARS ],
         vote[ MAX_TOKEN_CHARS ],
         arg[ MAX_TOKEN_CHARS ];
  char   name[ MAX_NAME_LENGTH ] = "";
  char   caller[ MAX_NAME_LENGTH ] = "";
  char   reason[ MAX_TOKEN_CHARS ];
  char   *creason;
  int    clientNum = -1;
  int    id = -1;
  team_t team;

  trap_Argv( 0, cmd, sizeof( cmd ) );
  trap_Argv( 1, vote, sizeof( vote ) );
  trap_Argv( 2, arg, sizeof( arg ) );
  creason = ConcatArgs( 3 );
  G_DecolorString( creason, reason, sizeof( reason ) );

  if( !Q_stricmp( cmd, "callteamvote" ) )
    team = ent->client->pers.teamSelection;
  else
    team = TEAM_NONE;
//only ADMF_CVA can do callvote to stop troll votes
  if( !g_allowVote.integer || !G_admin_permission( ent, ADMF_CVA )  )
  {
    trap_SendServerCommand( ent-g_entities,
      va( "print \"^5%s: Voting is not allowed for your admin level. You need level 2 or above. \n\"", cmd ) );
    return;
  }

  if( level.voteTime[ team ] )
  {
    trap_SendServerCommand( ent-g_entities,
      va( "print \"^5%s: a vote is already in progress\n\"", cmd ) );
    return;
  }

  if( level.voteExecuteTime[ team ] )
    G_ExecuteVote( team );

  level.voteDelay[ team ] = 0;
  level.voteThreshold[ team ] = 50;

  if( g_voteLimit.integer > 0 &&
    ent->client->pers.namelog->voteCount >= g_voteLimit.integer &&
    !G_admin_permission( ent, ADMF_NO_VOTE_LIMIT ) )
  {
    trap_SendServerCommand( ent-g_entities, va(
      "print \"^5%s: you have already called the maximum number of votes (%d)\n\"",
      cmd, g_voteLimit.integer ) );
    return;
  }

  // kick, mute, unmute, denybuild, allowbuild
  if( !Q_stricmp( vote, "kick" ) ||
      !Q_stricmp( vote, "mute" ) || !Q_stricmp( vote, "unmute" ) ||
      !Q_stricmp( vote, "denybuild" ) || !Q_stricmp( vote, "allowbuild" ) )
  {
    char err[ MAX_STRING_CHARS ];

    if( !arg[ 0 ] )
    {
      trap_SendServerCommand( ent-g_entities,
        va( "print \"%s: no target\n\"", cmd ) );
      return;
    }

    // with a little extra work only players from the right team are considered
    clientNum = G_ClientNumberFromString( arg, err, sizeof( err ) );

    if( clientNum == -1 )
    {
      ADMP( va( "%s: %s", cmd, err ) );
      return;
    }

    G_DecolorString( level.clients[ clientNum ].pers.netname, name, sizeof( name ) );
    id = level.clients[ clientNum ].pers.namelog->id;

    if( !Q_stricmp( vote, "kick" ) || !Q_stricmp( vote, "mute" ) ||
        !Q_stricmp( vote, "denybuild" ) )
    {
      if( G_admin_permission( g_entities + clientNum, ADMF_IMMUNITY ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: admin is immune\n\"", cmd ) );

        G_AdminMessage( NULL, va( S_COLOR_WHITE "%s" S_COLOR_YELLOW " attempted %s %s"
                                " on immune admin " S_COLOR_WHITE "%s" S_COLOR_YELLOW
                                " for: %s",
                                ent->client->pers.netname, cmd, vote, 
                                g_entities[ clientNum ].client->pers.netname, 
                                reason[ 0 ] ? reason : "no reason" ) );
        return;
      }

      if( team != TEAM_NONE &&
          ( ent->client->pers.teamSelection !=
            level.clients[ clientNum ].pers.teamSelection ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: player is not on your team\n\"", cmd ) );
        return;
      }

      if( !reason[ 0 ] && !G_admin_permission( ent, ADMF_UNACCOUNTABLE ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: You must provide a reason\n\"", cmd ) );
        return;
      }
    }
  }

  if( !Q_stricmp( vote, "kick" ) )
  {
    if( level.clients[ clientNum ].pers.localClient )
    {
      trap_SendServerCommand( ent-g_entities,
        va( "print \"%s: admin is immune\n\"", cmd ) );
      return;
    }

    Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
      "ban %s \"1s%s\" %s" S_COLOR_WHITE " called vote kick (%s)", level.clients[ clientNum ].pers.ip.str,
      g_adminTempBan.string, ent->client->pers.netname, reason );
    Com_sprintf( level.voteDisplayString[ team ],
      sizeof( level.voteDisplayString[ team ] ), "Kick player '%s'", name );
    if( reason[ 0 ] )
    {
      Q_strcat( level.voteDisplayString[ team ],
        sizeof( level.voteDisplayString[ team ] ), va( " for '%s'", reason ) );
    }
  }
  else if( !Q_stricmp( vote, "poll" ) )
  {
    char poll[ MAX_STRING_CHARS ];

    G_SingleLineString( ConcatArgs( 2 ), poll, sizeof( poll ) );

    if( strlen( poll ) == 0 )
    {
      trap_SendServerCommand( ent-g_entities,
        va( "print \"%s: please specify a poll question\n\"", cmd ) );
      return;
    }

    // use an exec string that allows log parsers to see the vote
    Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
      "echo poll \"%s\"", poll );
    Com_sprintf( level.voteDisplayString[ team ],
      sizeof( level.voteDisplayString[ team ] ),
      "(poll) %s^7", poll );
  }
  else if( team == TEAM_NONE )
  {
    if( !Q_stricmp( vote, "mute" ) )
    {
      if( level.clients[ clientNum ].pers.namelog->muted )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: player is already muted\n\"", cmd ) );
        return;
      }

      Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
        "mute %d", id );
      Com_sprintf( level.voteDisplayString[ team ],
        sizeof( level.voteDisplayString[ team ] ),
        "Mute player '%s'", name );
      if( reason[ 0 ] )
      {
        Q_strcat( level.voteDisplayString[ team ],
          sizeof( level.voteDisplayString[ team ] ), va( " for '%s'", reason ) );
      }
    }
    else if( !Q_stricmp( vote, "unmute" ) )
    {
      if( !level.clients[ clientNum ].pers.namelog->muted )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: player is not currently muted\n\"", cmd ) );
        return;
      }

      Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
        "unmute %d", id );
      Com_sprintf( level.voteDisplayString[ team ],
        sizeof( level.voteDisplayString[ team ] ),
        "Unmute player '%s'", name );
    }
    else if( !Q_stricmp( vote, "map_restart" ) )
    {
      if( (( level.time - level.startTime ) >= 300 * 1000 )
        && ( level.numPlayingClients > 0 && level.numConnectedClients > 1 ) )
      {
         trap_SendServerCommand( ent-g_entities, "print \"You cannot call for a map restart after 5 minutes.\n\"" );
         return;
      }
    
      strcpy( level.voteString[ team ], vote );
      strcpy( level.voteDisplayString[ team ], "Restart current map" );
      // map_restart comes with a default delay
    }
    else if( !Q_stricmp( vote, "map" ) )
    {
      if( (( level.time - level.startTime ) < 60 * 1000 ) )
      {
         trap_SendServerCommand( ent-g_entities, "print \"You cannot call for a map change in the first minute.\n\"" );
         return;
      }
	
      if( (( level.time - level.startTime ) >= 300 * 1000 )
        && ( level.numPlayingClients > 0 && level.numConnectedClients > 1 ) )
      {
         trap_SendServerCommand( ent-g_entities, "print \"You cannot call for a map change after 5 minutes\n\"" );
         return;
      }

    
      if( !G_MapExists( arg ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: 'maps/%s.bsp' could not be found on the server\n\"",
            cmd, arg ) );
        return;
      }

      if( reason[0] )
      {
	if( !G_LayoutExists( arg, reason ) )
	{
	  trap_SendServerCommand( ent-g_entities,
	    va( "print \"%s: 'layouts/%s/%s.dat' could not be found on the server\n\"",
	      cmd, arg, reason ) );
	  return;
	}
	Com_sprintf( level.voteString[ team ], sizeof( level.voteString ),
	  "%s \"%s\" \"%s\"", vote, arg, reason );
        Com_sprintf( level.voteDisplayString[ team ],
          sizeof( level.voteDisplayString[ team ] ),
          "Change to map '%s' (layout: '%s')", arg, reason );
      } else {
	Com_sprintf( level.voteString[ team ], sizeof( level.voteString ),
	  "%s \"%s\"", vote, arg );
        Com_sprintf( level.voteDisplayString[ team ],
          sizeof( level.voteDisplayString[ team ] ),
          "Change to map '%s'", arg );
      }
      level.voteDelay[ team ] = 3000;
    }
    else if( !Q_stricmp( vote, "nextmap" ) )
    {
      if( G_MapExists( g_nextMap.string ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: the next map is already set to '%s' layout '%s'\n\"",
	    cmd, g_nextMap.string, g_layouts.string ) );
        return;
      }

      if( !G_MapExists( arg ) )
      {
        trap_SendServerCommand( ent-g_entities,
          va( "print \"%s: 'maps/%s.bsp' could not be found on the server\n\"",
            cmd, arg ) );
        return;
      }

      if( reason[0] )
      {
	if( !G_LayoutExists( arg, reason ) )
	{
	  trap_SendServerCommand( ent-g_entities,
	    va( "print \"%s: 'layouts/%s/%s.dat' could not be found on the server\n\"",
	      cmd, arg, reason ) );
	  return;
	}
	Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
	  "set g_nextMap \"%s\";set g_layouts \"%s\"", arg, reason );
        Com_sprintf( level.voteDisplayString[ team ],
          sizeof( level.voteDisplayString[ team ] ),
	  "Set the ^1next^5 map to '%s' layout '%s'", arg, reason );
      } else {
	Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
	  "set g_nextMap \"%s\"", arg );
	Com_sprintf( level.voteDisplayString[ team ],
          sizeof( level.voteDisplayString[ team ] ),
          "Set the ^1next^5 map to '%s'", arg );
      }
    }
    else if( !Q_stricmp( vote, "sudden_death" ) )
    {
      if(!g_suddenDeathVotePercent.integer)
	  
      {
        trap_SendServerCommand( ent-g_entities,
              "print \"Sudden Death votes have been disabled\n\"" );
        return;
      }
      if( G_TimeTilSuddenDeath( ) <= 0 )
      {
        trap_SendServerCommand( ent - g_entities,
              va( "print \"callvote: Sudden Death has already begun\n\"") );
        return;
      }
      if( level.suddenDeathBeginTime > 0 &&
          G_TimeTilSuddenDeath() <= g_suddenDeathVoteDelay.integer * 1000 )
      {
        trap_SendServerCommand( ent - g_entities,
              va( "print \"callvote: Sudden Death is imminent\n\"") );
        return;
      }
      level.voteThreshold[ team ] = g_suddenDeathVotePercent.integer;
      Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
        "suddendeath %d", g_suddenDeathVoteDelay.integer );
      Com_sprintf( level.voteDisplayString[ team ],
                   sizeof( level.voteDisplayString[ team ] ),
                   "Begin sudden death in %d seconds",
                   g_suddenDeathVoteDelay.integer );
    }
    else if( !Q_stricmp( vote, "armageddon" ) )
    {
      int percent;

      if(!g_armageddonVotePercent.integer)
      {
        trap_SendServerCommand( ent-g_entities,
              "print \"Armageddon votes have been disabled\n\"" );
        return;
      }

      percent = atoi( arg );
      if( percent <= 0 || percent >100  )
        percent = g_armageddonPercent.integer;

      level.voteThreshold[ team ] = g_armageddonVotePercent.integer;
      Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
        "armageddon %d", percent );
      Com_sprintf( level.voteDisplayString[ team ],
                   sizeof( level.voteDisplayString[ team ] ),
                   "Armageddon (^5%d%% of ^1defensive buildings^5 is destroyed randomly.)",
                   percent );
    } 

    else
    {
      trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string\n\"" );
      trap_SendServerCommand( ent-g_entities, "print \"Valid vote commands are: "
        "map, nextmap, map_restart, sudden_death, armageddon, kick, poll, mute and unmute\n" );
      return;
    }
  }
  else if( !Q_stricmp( vote, "denybuild" ) )
  {
    if( level.clients[ clientNum ].pers.namelog->denyBuild )
    {
      trap_SendServerCommand( ent-g_entities,
        va( "print \"%s: player already lost building rights\n\"", cmd ) );
      return;
    }

    Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
      "denybuild %d", id );
    Com_sprintf( level.voteDisplayString[ team ],
      sizeof( level.voteDisplayString[ team ] ),
      "Take away building rights from '%s'", name );
    if( reason[ 0 ] )
    {
      Q_strcat( level.voteDisplayString[ team ],
        sizeof( level.voteDisplayString[ team ] ), va( " for '%s'", reason ) );
    }
  }
  else if( !Q_stricmp( vote, "allowbuild" ) )
  {
    if( !level.clients[ clientNum ].pers.namelog->denyBuild )
    {
      trap_SendServerCommand( ent-g_entities,
        va( "print \"%s: player already has building rights\n\"", cmd ) );
      return;
    }

    Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
      "allowbuild %d", id );
    Com_sprintf( level.voteDisplayString[ team ],
      sizeof( level.voteDisplayString[ team ] ),
      "Allow '%s' to build", name );
  }
  else if( !Q_stricmp( vote, "admitdefeat" ) )
  {
    if( ( ( level.time - level.startTime ) < 60 * 10000 ) )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You cannot admit defeat in the first 10 minutes.\n\"" );
      return;
    }

    Com_sprintf( level.voteString[ team ], sizeof( level.voteString[ team ] ),
      "admitdefeat %d", team );
    strcpy( level.voteDisplayString[ team ], "Admit Defeat" );
  }
  else
  {
    trap_SendServerCommand( ent-g_entities, "print \"Invalid vote string\n\"" );
    trap_SendServerCommand( ent-g_entities,
       "print \"Valid team vote commands are: "
       "kick, poll, denybuild, allowbuild and admitdefeat\n\"" );
    return;
  }

  G_LogPrintf( "%s: %d \"%s" S_COLOR_WHITE "\": %s\n",
    team == TEAM_NONE ? "CallVote" : "CallTeamVote",
    ent - g_entities, ent->client->pers.netname, level.voteString[ team ] );

  if( team == TEAM_NONE )
  {
    trap_SendServerCommand( -1, va( "print \"^5[VOTE]^7 %s" S_COLOR_WHITE
      " called a vote: %s\n\"", ent->client->pers.netname,
      level.voteDisplayString[ team ] ) );
  }
  else
  {
    int i;

    for( i = 0 ; i < level.maxclients ; i++ )
    {
      if( level.clients[ i ].pers.connected == CON_CONNECTED )
      {
        if( level.clients[ i ].pers.teamSelection == team ||
            ( level.clients[ i ].pers.teamSelection == TEAM_NONE &&
            G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) ) )
        {
          trap_SendServerCommand( i, va( "print \"^5[TEAMVOTE]^7 %s" S_COLOR_WHITE
            " called a team vote: %s\n\"", ent->client->pers.netname,
            level.voteDisplayString[ team ] ) );
        }
        else if( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
        {
          trap_SendServerCommand( i, va( "chat -1 %d \"" S_COLOR_YELLOW "%s" 
            S_COLOR_YELLOW " called a team vote (%ss): %s\"", SAY_ADMINS,
            ent->client->pers.netname, BG_TeamName( team ), 
            level.voteDisplayString[ team ] ) );
        }
      }
    }
  }

  G_DecolorString( ent->client->pers.netname, caller, sizeof( caller ) );

  level.voteTime[ team ] = level.time;
  trap_SetConfigstring( CS_VOTE_TIME + team,
    va( "%d", level.voteTime[ team ] ) );
  trap_SetConfigstring( CS_VOTE_STRING + team,
    level.voteDisplayString[ team ] );
  trap_SetConfigstring( CS_VOTE_CALLER + team,
    caller );

  ent->client->pers.namelog->voteCount++;

  // don't automatically vote yes for polls
  if( !Q_stricmp( vote, "poll" ) )
    return;

  ent->client->pers.vote |= 1 << team;
  G_Vote( ent, team, qtrue );
}

/*
==================
Cmd_Vote_f
==================
*/
void Cmd_Vote_f( gentity_t *ent )
{
  char cmd[ MAX_TOKEN_CHARS ], vote[ MAX_TOKEN_CHARS ];
  team_t team = ent->client->pers.teamSelection;

  trap_Argv( 0, cmd, sizeof( cmd ) );
  if( Q_stricmp( cmd, "teamvote" ) )
    team = TEAM_NONE;

  if( !level.voteTime[ team ] )
  {
    trap_SendServerCommand( ent-g_entities,
      va( "print \"%s: no vote in progress\n\"", cmd ) );
    return;
  }

  if( ent->client->pers.voted & ( 1 << team ) )
  {
    trap_SendServerCommand( ent-g_entities,
      va( "print \"%s: vote already cast\n\"", cmd ) );
    return;
  }


  //trap_SendServerCommand( ent-g_entities,
  //  va( "print \"%s: vote cast\n\"", cmd ) );

  trap_Argv( 1, vote, sizeof( vote ) );
  

  if( vote[ 0 ] == 'y' )
  {
    ent->client->pers.vote |= 1 << team;
    
    if (team != TEAM_NONE)//Je teamvote
      G_TeamCommand( ent->client->pers.teamSelection, va( "print \"^5Teamvote:^7 %s ^7voted ^2YES^7\n\"", ent->client->pers.netname ) );
    else
      trap_SendServerCommand( -1, va("print \"^7Vote: %s ^7voted ^2YES^7\n\"", ent->client->pers.netname ) );
    
    //G_LogPrintf("Vote: %s%s ^7voted YES\n", marking(ent), ent->client->pers.netname );
  }
  else
  {
    ent->client->pers.vote &= ~( 1 << team );

    if (team != TEAM_NONE)//Je teamvote
      G_TeamCommand( ent->client->pers.teamSelection, va( "print \"^5Teamvote:^7 %s ^7voted ^1NO^7\n\"", ent->client->pers.netname ) );
    else
      trap_SendServerCommand( -1, va("print \"^7Vote: %s ^7voted ^1NO^7\n\"", ent->client->pers.netname ) );
    
    //G_LogPrintf("Vote: %s%s ^7voted NO\n", marking(ent), ent->client->pers.netname );
  }
    
  G_Vote( ent, team, qtrue );
}


/*
=================
Cmd_SetViewpos_f
=================
*/
void Cmd_SetViewpos_f( gentity_t *ent )
{
  vec3_t  origin, angles;
  char    buffer[ MAX_TOKEN_CHARS ];
  int     i;

  if( trap_Argc( ) != 5 )
  {
    trap_SendServerCommand( ent-g_entities, "print \"usage: setviewpos x y z yaw\n\"" );
    return;
  }

  VectorClear( angles );

  for( i = 0; i < 3; i++ )
  {
    trap_Argv( i + 1, buffer, sizeof( buffer ) );
    origin[ i ] = atof( buffer );
  }

  trap_Argv( 4, buffer, sizeof( buffer ) );
  angles[ YAW ] = atof( buffer );

  TeleportPlayer( ent, origin, angles );
}

#define AS_OVER_RT3         ((ALIENSENSE_RANGE*0.5f)/M_ROOT3)

static qboolean G_RoomForClassChange( gentity_t *ent, class_t class,
                                      vec3_t newOrigin )
{
  vec3_t    fromMins, fromMaxs;
  vec3_t    toMins, toMaxs;
  vec3_t    temp;
  trace_t   tr;
  float     nudgeHeight;
  float     maxHorizGrowth;
  class_t   oldClass = ent->client->ps.stats[ STAT_CLASS ];

  BG_ClassBoundingBox( oldClass, fromMins, fromMaxs, NULL, NULL, NULL );
  BG_ClassBoundingBox( class, toMins, toMaxs, NULL, NULL, NULL );

  VectorCopy( ent->s.origin, newOrigin );

  // find max x/y diff
  maxHorizGrowth = toMaxs[ 0 ] - fromMaxs[ 0 ];
  if( toMaxs[ 1 ] - fromMaxs[ 1 ] > maxHorizGrowth )
    maxHorizGrowth = toMaxs[ 1 ] - fromMaxs[ 1 ];
  if( toMins[ 0 ] - fromMins[ 0 ] > -maxHorizGrowth )
    maxHorizGrowth = -( toMins[ 0 ] - fromMins[ 0 ] );
  if( toMins[ 1 ] - fromMins[ 1 ] > -maxHorizGrowth )
    maxHorizGrowth = -( toMins[ 1 ] - fromMins[ 1 ] );

  if( maxHorizGrowth > 0.0f )
  {
    // test by moving the player up the max required on a 60 degree slope
    nudgeHeight = maxHorizGrowth * 2.0f;
  }
  else
  {
    // player is shrinking, so there's no need to nudge them upwards
    nudgeHeight = 0.0f;
  }

  // find what the new origin would be on a level surface
  newOrigin[ 2 ] -= toMins[ 2 ] - fromMins[ 2 ];

  //compute a place up in the air to start the real trace
  VectorCopy( newOrigin, temp );
  temp[ 2 ] += nudgeHeight;
  trap_Trace( &tr, newOrigin, toMins, toMaxs, temp, ent->s.number, MASK_PLAYERSOLID );

  //trace down to the ground so that we can evolve on slopes
  VectorCopy( newOrigin, temp );
  temp[ 2 ] += ( nudgeHeight * tr.fraction );
  trap_Trace( &tr, temp, toMins, toMaxs, newOrigin, ent->s.number, MASK_PLAYERSOLID );
  VectorCopy( tr.endpos, newOrigin );

  //make REALLY sure
  trap_Trace( &tr, newOrigin, toMins, toMaxs, newOrigin,
    ent->s.number, MASK_PLAYERSOLID );

  //check there is room to evolve
  return ( !tr.startsolid && tr.fraction == 1.0f );
}

/*
=================
Cmd_Class_f
=================
*/
void Cmd_Class_f( gentity_t *ent )
{
  char      s[ MAX_TOKEN_CHARS ];
  int       clientNum;
  int       i;
  vec3_t    infestOrigin;
  class_t   currentClass = ent->client->pers.classSelection;
  class_t   newClass;
  int       entityList[ MAX_GENTITIES ];
  vec3_t    range = { AS_OVER_RT3, AS_OVER_RT3, AS_OVER_RT3 };
  vec3_t    mins, maxs;
  int       num;
  gentity_t *other;
  int       oldBoostTime = -1;
  vec3_t    oldVel;

  clientNum = ent->client - level.clients;
  trap_Argv( 1, s, sizeof( s ) );
  newClass = BG_ClassByName( s )->number;

  if( ent->client->sess.spectatorState != SPECTATOR_NOT )
  {
    if( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
      G_StopFollowing( ent );
    if( ent->client->pers.teamSelection == TEAM_ALIENS )
    {
//      if( newClass != PCL_ALIEN_BUILDER0 &&
//          newClass != PCL_ALIEN_BUILDER0_UPG &&
//          newClass != PCL_ALIEN_LEVEL0 )
// Allow grangers only before SD, or if there is no OM.

      if( !( ( ( ( newClass == PCL_ALIEN_BUILDER0 ) ||

                 ( newClass == PCL_ALIEN_BUILDER0_UPG ) ) &&

               ( ( G_TimeTilSuddenDeath() > 0 ) ||

                 ( G_Overmind() == NULL  ) ) ) ||

             ( newClass == PCL_ALIEN_LEVEL0 ) ) )
      {
        G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSNOTSPAWN, newClass );
        return;
      }

 //     if( !BG_ClassIsAllowed( newClass ) )
	  if( !G_ClassIsAllowed( newClass ) )
      {
        G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSNOTALLOWED, newClass );
        return;
      }

      if( !BG_ClassAllowedInStage( newClass, g_alienStage.integer ) )
      {
        G_TriggerMenuArgs( ent->client->ps.clientNum, MN_A_CLASSNOTATSTAGE, newClass );
        return;
      }

      // spawn from an egg
      if( G_PushSpawnQueue( &level.alienSpawnQueue, clientNum ) )
      {
        ent->client->pers.classSelection = newClass;
        ent->client->ps.stats[ STAT_CLASS ] = newClass;
      }
    }
    else if( ent->client->pers.teamSelection == TEAM_HUMANS )
    {
      //set the item to spawn with
      if( !Q_stricmp( s, BG_Weapon( WP_MACHINEGUN )->name ) &&
         // BG_WeaponIsAllowed( WP_MACHINEGUN ) )
		  G_WeaponIsAllowed( WP_MACHINEGUN ) )
      {
        ent->client->pers.humanItemSelection = WP_MACHINEGUN;
      }
      else if( !Q_stricmp( s, BG_Weapon( WP_HBUILD )->name ) &&
         //      BG_WeaponIsAllowed( WP_HBUILD ) )
		 G_WeaponIsAllowed( WP_HBUILD ) )
      {
        ent->client->pers.humanItemSelection = WP_HBUILD;
      }
      else
      {
        G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNSPAWNITEM );
        return;
      }
      // spawn from a telenode
      if( G_PushSpawnQueue( &level.humanSpawnQueue, clientNum ) )
      {
        ent->client->pers.classSelection = PCL_HUMAN;
        ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN;
      }
    }
    return;
  }

  if( ent->health <= 0 )
    return;

  if( ent->client->pers.teamSelection == TEAM_ALIENS )
  {
    if( newClass == PCL_NONE )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_A_UNKNOWNCLASS );
      return;
    }

    //if we are not currently spectating, we are attempting evolution
    if( ent->client->pers.classSelection != PCL_NONE )
    {
      int cost;

      //check that we have an overmind
      if( !G_Overmind( ) )
      {
        G_TriggerMenu( clientNum, MN_A_NOOVMND_EVOLVE );
        return;
      }

      //check there are no humans nearby
	  //but allow if there is creep
      VectorAdd( ent->client->ps.origin, range, maxs );
      VectorSubtract( ent->client->ps.origin, range, mins );

      num = trap_EntitiesInBox( mins, maxs, entityList, MAX_GENTITIES );
      for( i = 0; i < num; i++ )
      {
        other = &g_entities[ entityList[ i ] ];

        if( ( other->client && other->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS ) ||
            ( other->s.eType == ET_BUILDABLE && other->buildableTeam == TEAM_HUMANS &&
              other->powered ) )
        {
		if( G_IsCreepHere( ent->client->ps.origin ) )
            break; // always allow when on creep 
          G_TriggerMenu( clientNum, MN_A_TOOCLOSE );
          return;
        }
      }

      //check that we are not wallwalking
      if( ent->client->ps.eFlags & EF_WALLCLIMB )
      {
        G_TriggerMenu( clientNum, MN_A_EVOLVEWALLWALK );
        return;
      }

      //guard against selling the HBUILD weapons exploit
      if( ent->client->sess.spectatorState == SPECTATOR_NOT &&
          ( currentClass == PCL_ALIEN_BUILDER0 ||
            currentClass == PCL_ALIEN_BUILDER0_UPG ) &&
          ent->client->ps.stats[ STAT_MISC ] > 0 )
      {
        G_TriggerMenu( ent->client->ps.clientNum, MN_A_EVOLVEBUILDTIMER );
        return;
      }

      cost = BG_ClassCanEvolveFromTo( currentClass, newClass,
                                      ent->client->pers.credit,
                                      g_alienStage.integer, 0 );

     // if( G_RoomForClassChange( ent, newClass, infestOrigin ) )
	  if( G_RoomForClassChange( ent, newClass, infestOrigin ) && G_ClassIsAllowed( newClass ) )
      {
        if( cost >= 0 )
        {

          ent->client->pers.evolveHealthFraction = (float)ent->client->ps.stats[ STAT_HEALTH ] /
            (float)BG_Class( currentClass )->health;

          if( ent->client->pers.evolveHealthFraction < 0.0f )
            ent->client->pers.evolveHealthFraction = 0.0f;
          else if( ent->client->pers.evolveHealthFraction > 1.0f )
            ent->client->pers.evolveHealthFraction = 1.0f;

          //remove credit
          G_AddCreditToClient( ent->client, -cost, qtrue );
          ent->client->pers.classSelection = newClass;
          ClientUserinfoChanged( clientNum, qfalse );
          VectorCopy( infestOrigin, ent->s.pos.trBase );
          VectorCopy( ent->client->ps.velocity, oldVel );

          if( ent->client->ps.stats[ STAT_STATE ] & SS_BOOSTED )
            oldBoostTime = ent->client->boostedTime;

          ClientSpawn( ent, ent, ent->s.pos.trBase, ent->s.apos.trBase );

          VectorCopy( oldVel, ent->client->ps.velocity );
          if( oldBoostTime > 0 )
          {
            ent->client->boostedTime = oldBoostTime;
            ent->client->ps.stats[ STAT_STATE ] |= SS_BOOSTED;
          }
        }
        else
          G_TriggerMenuArgs( clientNum, MN_A_CANTEVOLVE, newClass );
      }
      else
        G_TriggerMenu( clientNum, MN_A_NOEROOM );
    }
  }
  else if( ent->client->pers.teamSelection == TEAM_HUMANS )
    G_TriggerMenu( clientNum, MN_H_DEADTOCLASS );
}


/*
=================
Cmd_Destroy_f
=================
*/
void Cmd_Destroy_f( gentity_t *ent )
{
  vec3_t      viewOrigin, forward, end;
  trace_t     tr;
  gentity_t   *traceEnt;
  char        cmd[ 12 ];
  qboolean    deconstruct = qtrue;
  qboolean    lastSpawn = qfalse;

  if( ent->client->pers.namelog->denyBuild )
  {
    G_TriggerMenu( ent->client->ps.clientNum, MN_B_REVOKED );
    return;
  }

  trap_Argv( 0, cmd, sizeof( cmd ) );
  if( Q_stricmp( cmd, "destroy" ) == 0 )
    deconstruct = qfalse;

  BG_GetClientViewOrigin( &ent->client->ps, viewOrigin );
  AngleVectors( ent->client->ps.viewangles, forward, NULL, NULL );
  VectorMA( viewOrigin, 100, forward, end );

  trap_Trace( &tr, viewOrigin, NULL, NULL, end, ent->s.number, MASK_PLAYERSOLID );
  traceEnt = &g_entities[ tr.entityNum ];

  if( tr.fraction < 1.0f &&
      ( traceEnt->s.eType == ET_BUILDABLE ) &&
      ( traceEnt->buildableTeam == ent->client->pers.teamSelection ) &&
      ( ( ent->client->ps.weapon >= WP_ABUILD ) &&
        ( ent->client->ps.weapon <= WP_HBUILD ) ) )
  {
    // Always let the builder prevent the explosion
    if( traceEnt->health <= 0 )
    {
      G_QueueBuildPoints( traceEnt );
      G_RewardAttackers( traceEnt );
      G_FreeEntity( traceEnt );
      return;
    }

    // Cancel deconstruction (unmark)
    if( deconstruct && g_markDeconstruct.integer && traceEnt->deconstruct )
    {
      traceEnt->deconstruct = qfalse;
      return;
    }

    // Prevent destruction of the last spawn
    if( ent->client->pers.teamSelection == TEAM_ALIENS &&
        traceEnt->s.modelindex == BA_A_SPAWN )
    {
      if( level.numAlienSpawns <= 1 )
        lastSpawn = qtrue;
    }
    else if( ent->client->pers.teamSelection == TEAM_HUMANS &&
             traceEnt->s.modelindex == BA_H_SPAWN )
    {
      if( level.numHumanSpawns <= 1 )
        lastSpawn = qtrue;
    }

    if( lastSpawn && !g_cheats.integer &&
        !g_markDeconstruct.integer )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_B_LASTSPAWN );
      return;
    }

    // Don't allow destruction of buildables that cannot be rebuilt
    if( G_TimeTilSuddenDeath( ) <= 0 )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_B_SUDDENDEATH );
      return;
    }
    
    // pokud nema jmeno, nemuze bourat
    if (g_newbieNamePrefix.string[ 0 ] && !Q_stricmpn( ent->client->pers.netname, g_newbieNamePrefix.string, strlen(g_newbieNamePrefix.string)))
    {
      trap_SendServerCommand( ent-g_entities,
        "print \"Take a nick before trying to deconstruct. Type: /name your_nick\n\"" );
      return; 
    }

    if( !g_markDeconstruct.integer ||
        ( ent->client->pers.teamSelection == TEAM_HUMANS &&
          !G_FindPower( traceEnt, qtrue ) ) )
    {
      if( ent->client->ps.stats[ STAT_MISC ] > 0 )
      {
        G_AddEvent( ent, EV_BUILD_DELAY, ent->client->ps.clientNum );
        return;
      }
    }

    if( traceEnt->health > 0 )
    {
      if( !deconstruct )
      {
        G_Damage( traceEnt, ent, ent, forward, tr.endpos,
                  traceEnt->health, 0, MOD_SUICIDE );
      }
      else if( g_markDeconstruct.integer &&
               ( ent->client->pers.teamSelection != TEAM_HUMANS ||
                 G_FindPower( traceEnt , qtrue ) || lastSpawn ) )
      {
        traceEnt->deconstruct     = qtrue; // Mark buildable for deconstruction
        traceEnt->deconstructTime = level.time;
      }
      else
      {
        if( !g_cheats.integer ) // add a bit to the build timer
        {
            ent->client->ps.stats[ STAT_MISC ] +=
              BG_Buildable( traceEnt->s.modelindex )->buildTime / 4;
        }
        G_Damage( traceEnt, ent, ent, forward, tr.endpos,
                  traceEnt->health, 0, MOD_DECONSTRUCT );
        G_FreeEntity( traceEnt );
      }
    }
  }
}

/*
=================
Cmd_ActivateItem_f

Activate an item
=================
*/
void Cmd_ActivateItem_f( gentity_t *ent )
{
  char  s[ MAX_TOKEN_CHARS ];
  int   upgrade, weapon;

  trap_Argv( 1, s, sizeof( s ) );

  // "weapon" aliased to whatever weapon you have
  if( !Q_stricmp( "weapon", s ) )
  {
    if( ent->client->ps.weapon == WP_BLASTER &&
        BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      G_ForceWeaponChange( ent, WP_NONE );
    return;
  }

  upgrade = BG_UpgradeByName( s )->number;
  weapon = BG_WeaponByName( s )->number;

  if( upgrade != UP_NONE && BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
    BG_ActivateUpgrade( upgrade, ent->client->ps.stats );
  else if( weapon != WP_NONE &&
           BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
  {
    if( ent->client->ps.weapon != weapon &&
        BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      G_ForceWeaponChange( ent, weapon );
  }
  else
    trap_SendServerCommand( ent-g_entities, va( "print \"You don't have the %s\n\"", s ) );
}


/*
=================
Cmd_DeActivateItem_f

Deactivate an item
=================
*/
void Cmd_DeActivateItem_f( gentity_t *ent )
{
  char      s[ MAX_TOKEN_CHARS ];
  upgrade_t upgrade;

  trap_Argv( 1, s, sizeof( s ) );
  upgrade = BG_UpgradeByName( s )->number;

  if( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
    BG_DeactivateUpgrade( upgrade, ent->client->ps.stats );
  else
    trap_SendServerCommand( ent-g_entities, va( "print \"You don't have the %s\n\"", s ) );
}


/*
=================
Cmd_ToggleItem_f
=================
*/
void Cmd_ToggleItem_f( gentity_t *ent )
{
  char      s[ MAX_TOKEN_CHARS ];
  weapon_t  weapon;
  upgrade_t upgrade;

  trap_Argv( 1, s, sizeof( s ) );
  upgrade = BG_UpgradeByName( s )->number;
  weapon = BG_WeaponByName( s )->number;

  if( weapon != WP_NONE )
  {
    if( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      return;

    //special case to allow switching between
    //the blaster and the primary weapon
    if( ent->client->ps.weapon != WP_BLASTER )
      weapon = WP_BLASTER;
    else
      weapon = WP_NONE;

    G_ForceWeaponChange( ent, weapon );
  }
  else if( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
  {
    if( BG_UpgradeIsActive( upgrade, ent->client->ps.stats ) )
      BG_DeactivateUpgrade( upgrade, ent->client->ps.stats );
    else
      BG_ActivateUpgrade( upgrade, ent->client->ps.stats );
  }
  else
    trap_SendServerCommand( ent-g_entities, va( "print \"You don't have the %s\n\"", s ) );
}

/*
=================
Cmd_Buy_f
=================
*/
void Cmd_Buy_f( gentity_t *ent )
{
  char s[ MAX_TOKEN_CHARS ];
  weapon_t  weapon;
  upgrade_t upgrade;
  qboolean  energyOnly;

  trap_Argv( 1, s, sizeof( s ) );

  weapon = BG_WeaponByName( s )->number;
  upgrade = BG_UpgradeByName( s )->number;

  // Only give energy from reactors or repeaters
  if( G_BuildableRange( ent->client->ps.origin, 100, BA_H_ARMOURY))
    energyOnly = qfalse;
  else if( upgrade == UP_AMMO &&
           BG_Weapon( ent->client->ps.stats[ STAT_WEAPON ] )->usesEnergy &&
           ( G_BuildableRange( ent->client->ps.origin, 100, BA_H_REACTOR ) ||
             G_BuildableRange( ent->client->ps.origin, 100, BA_H_REPEATER ) ) )
    energyOnly = qtrue;
	  else if( upgrade == UP_AMMO &&
           !BG_Weapon( ent->client->ps.stats[ STAT_WEAPON ] )->usesEnergy &&
           ( G_BuildableRange( ent->client->ps.origin, 100, BA_H_LADDER) ) )
    energyOnly = qfalse;
  else
  {
    if( upgrade == UP_AMMO &&
        BG_Weapon( ent->client->ps.weapon )->usesEnergy )
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOENERGYAMMOHERE );
    else
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOARMOURYHERE );
    return;
  }

  if( weapon != WP_NONE )
  {
    //already got this?
    if( BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_ITEMHELD );
      return;
    }

    // Only humans can buy stuff
    if( BG_Weapon( weapon )->team != TEAM_HUMANS )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy alien items\n\"" );
      return;
    }

    //are we /allowed/ to buy this?
    if( !BG_Weapon( weapon )->purchasable )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy this item\n\"" );
      return;
    }

    //are we /allowed/ to buy this?
   // if( !BG_WeaponAllowedInStage( weapon, g_humanStage.integer ) || !BG_WeaponIsAllowed( weapon ) )
	if( !BG_WeaponAllowedInStage( weapon, g_humanStage.integer ) || !G_WeaponIsAllowed( weapon ) )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy this item\n\"" );
      return;
    }

    //can afford this?
    if( BG_Weapon( weapon )->price > (short)ent->client->pers.credit )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOFUNDS );
      return;
    }

    //have space to carry this?
    if( BG_Weapon( weapon )->slots & BG_SlotsForInventory( ent->client->ps.stats ) )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOSLOTS );
      return;
    }

    // In some instances, weapons can't be changed
    if( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      return;

    ent->client->ps.stats[ STAT_WEAPON ] = weapon;
    ent->client->ps.ammo = BG_Weapon( weapon )->maxAmmo;
    ent->client->ps.clips = BG_Weapon( weapon )->maxClips;

    if( BG_Weapon( weapon )->usesEnergy &&
        BG_InventoryContainsUpgrade( UP_BATTPACK, ent->client->ps.stats ) )
      ent->client->ps.ammo *= BATTPACK_MODIFIER;

    G_ForceWeaponChange( ent, weapon );

    //set build delay/pounce etc to 0
    ent->client->ps.stats[ STAT_MISC ] = 0;

    //subtract from funds
    G_AddCreditToClient( ent->client, -(short)BG_Weapon( weapon )->price, qfalse );
  }
  else if( upgrade != UP_NONE )
  {
    //already got this?
    if( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_ITEMHELD );
      return;
    }

    //can afford this?
    if( BG_Upgrade( upgrade )->price > (short)ent->client->pers.credit )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOFUNDS );
      return;
    }

    //have space to carry this?
    if( BG_Upgrade( upgrade )->slots & BG_SlotsForInventory( ent->client->ps.stats ) )
    {
      G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOSLOTS );
      return;
    }

    // Only humans can buy stuff
    if( BG_Upgrade( upgrade )->team != TEAM_HUMANS )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy alien items\n\"" );
      return;
    }

    //are we /allowed/ to buy this?
    if( !BG_Upgrade( upgrade )->purchasable )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy this item\n\"" );
      return;
    }

    //are we /allowed/ to buy this?
    if( !BG_UpgradeAllowedInStage( upgrade, g_humanStage.integer ) || !BG_UpgradeIsAllowed( upgrade ) )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't buy this item\n\"" );
      return;
    }

    if( upgrade == UP_AMMO )
      G_GiveClientMaxAmmo( ent, energyOnly );
    else
    {
      if( upgrade == UP_BATTLESUIT )
      {
        vec3_t newOrigin;

        if( !G_RoomForClassChange( ent, PCL_HUMAN_BSUIT, newOrigin ) )
        {
          G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMBSUITON );
          return;
        }
        VectorCopy( newOrigin, ent->client->ps.origin );
        ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN_BSUIT;
        ent->client->pers.classSelection = PCL_HUMAN_BSUIT;
        ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
      }

      //add to inventory
      BG_AddUpgradeToInventory( upgrade, ent->client->ps.stats );
    }

    if( upgrade == UP_BATTPACK )
      G_GiveClientMaxAmmo( ent, qtrue );

    if( upgrade == UP_CLOAK )
    {
      ent->client->cloakReady = qtrue;
      ent->client->ps.eFlags &= ~EF_MOVER_STOP;
    }

    //subtract from funds
    G_AddCreditToClient( ent->client, -(short)BG_Upgrade( upgrade )->price, qfalse );
  }
  else
    G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNITEM );

  //update ClientInfo
  ClientUserinfoChanged( ent->client->ps.clientNum, qfalse );
}


/*
=================
Cmd_Sell_f
=================
*/
void Cmd_Sell_f( gentity_t *ent )
{
  char      s[ MAX_TOKEN_CHARS ];
  int       i;
  weapon_t  weapon;
  upgrade_t upgrade;

  trap_Argv( 1, s, sizeof( s ) );

  //no armoury nearby
  if( !G_BuildableRange( ent->client->ps.origin, 100, BA_H_ARMOURY ) )
  {
    G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOARMOURYHERE );
    return;
  }

  if( !Q_stricmpn( s, "weapon", 6 ) )
    weapon = ent->client->ps.stats[ STAT_WEAPON ];
  else
    weapon = BG_WeaponByName( s )->number;

  upgrade = BG_UpgradeByName( s )->number;

  if( weapon != WP_NONE )
  {
    weapon_t selected = BG_GetPlayerWeapon( &ent->client->ps );

    if( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      return;

    //are we /allowed/ to sell this?
    if( !BG_Weapon( weapon )->purchasable )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't sell this weapon\n\"" );
      return;
    }

    //remove weapon if carried
    if( BG_InventoryContainsWeapon( weapon, ent->client->ps.stats ) )
    {
      //guard against selling the HBUILD weapons exploit
      if( weapon == WP_HBUILD && ent->client->ps.stats[ STAT_MISC ] > 0 )
      {
        G_TriggerMenu( ent->client->ps.clientNum, MN_H_ARMOURYBUILDTIMER );
        return;
      }

      ent->client->ps.stats[ STAT_WEAPON ] = WP_NONE;
      // Cancel ghost buildables
      ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;

      //add to funds
      G_AddCreditToClient( ent->client, (short)BG_Weapon( weapon )->price, qfalse );
    }

    //if we have this weapon selected, force a new selection
    if( weapon == selected )
      G_ForceWeaponChange( ent, WP_NONE );
  }
  else if( upgrade != UP_NONE )
  {
    //are we /allowed/ to sell this?
    if( !BG_Upgrade( upgrade )->purchasable )
    {
      trap_SendServerCommand( ent-g_entities, "print \"You can't sell this item\n\"" );
      return;
    }
    //remove upgrade if carried
    if( BG_InventoryContainsUpgrade( upgrade, ent->client->ps.stats ) )
    {
      // shouldn't really need to test for this, but just to be safe
      if( upgrade == UP_BATTLESUIT )
      {
        vec3_t newOrigin;

        if( !G_RoomForClassChange( ent, PCL_HUMAN, newOrigin ) )
        {
          G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMBSUITOFF );
          return;
        }
        VectorCopy( newOrigin, ent->client->ps.origin );
        ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN;
        ent->client->pers.classSelection = PCL_HUMAN;
        ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
      }

      //add to inventory
      BG_RemoveUpgradeFromInventory( upgrade, ent->client->ps.stats );

      if( upgrade == UP_BATTPACK )
        G_GiveClientMaxAmmo( ent, qtrue );


      if( upgrade == UP_CLOAK )
      {
        ent->client->cloakReady = qfalse;
        ent->client->ps.eFlags &= ~EF_MOVER_STOP;
      }
      //add to funds
      G_AddCreditToClient( ent->client, (short)BG_Upgrade( upgrade )->price, qfalse );
    }
  }
  else if( !Q_stricmp( s, "weapons" ) )
  {
    weapon_t selected = BG_GetPlayerWeapon( &ent->client->ps );

    if( !BG_PlayerCanChangeWeapon( &ent->client->ps ) )
      return;

    for( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
    {
      //guard against selling the HBUILD weapons exploit
      if( i == WP_HBUILD && ent->client->ps.stats[ STAT_MISC ] > 0 )
      {
        G_TriggerMenu( ent->client->ps.clientNum, MN_H_ARMOURYBUILDTIMER );
        continue;
      }

      if( BG_InventoryContainsWeapon( i, ent->client->ps.stats ) &&
          BG_Weapon( i )->purchasable )
      {
        ent->client->ps.stats[ STAT_WEAPON ] = WP_NONE;

        //add to funds
        G_AddCreditToClient( ent->client, (short)BG_Weapon( i )->price, qfalse );
      }

      //if we have this weapon selected, force a new selection
      if( i == selected )
        G_ForceWeaponChange( ent, WP_NONE );
    }
  }
  else if( !Q_stricmp( s, "upgrades" ) )
  {
    for( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
    {
      //remove upgrade if carried
      if( BG_InventoryContainsUpgrade( i, ent->client->ps.stats ) &&
          BG_Upgrade( i )->purchasable )
      {

        // shouldn't really need to test for this, but just to be safe
        if( i == UP_BATTLESUIT )
        {
          vec3_t newOrigin;

          if( !G_RoomForClassChange( ent, PCL_HUMAN, newOrigin ) )
          {
            G_TriggerMenu( ent->client->ps.clientNum, MN_H_NOROOMBSUITOFF );
            continue;
          }
          VectorCopy( newOrigin, ent->client->ps.origin );
          ent->client->ps.stats[ STAT_CLASS ] = PCL_HUMAN;
          ent->client->pers.classSelection = PCL_HUMAN;
          ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
        }

        BG_RemoveUpgradeFromInventory( i, ent->client->ps.stats );

        if( i == UP_BATTPACK )
          G_GiveClientMaxAmmo( ent, qtrue );

        if( i == UP_CLOAK )
        {
          ent->client->cloakReady = qfalse;
          ent->client->ps.eFlags &= ~EF_MOVER_STOP;
        }

        //add to funds
        G_AddCreditToClient( ent->client, (short)BG_Upgrade( i )->price, qfalse );
      }
    }
  }
  else
    G_TriggerMenu( ent->client->ps.clientNum, MN_H_UNKNOWNITEM );

  //update ClientInfo
  ClientUserinfoChanged( ent->client->ps.clientNum, qfalse );
}


/*
=================
Cmd_Build_f
=================
*/
void Cmd_Build_f( gentity_t *ent )
{
  char          s[ MAX_TOKEN_CHARS ];
  buildable_t   buildable;
  float         dist;
  vec3_t        origin, normal;
  team_t        team;

  if( ent->client->pers.namelog->denyBuild )
  {
    G_TriggerMenu( ent->client->ps.clientNum, MN_B_REVOKED );
    return;
  }

  if( ent->client->pers.teamSelection == level.surrenderTeam )
  {
    G_TriggerMenu( ent->client->ps.clientNum, MN_B_SURRENDER );
    return;
  }

  trap_Argv( 1, s, sizeof( s ) );

  buildable = BG_BuildableByName( s )->number;

  if( G_TimeTilSuddenDeath( ) <= 0 && buildable != BA_H_REACTOR && buildable != BA_A_OVERMIND )
  {
    G_TriggerMenu( ent->client->ps.clientNum, MN_B_SUDDENDEATH );
    return;
  }

  team = ent->client->ps.stats[ STAT_TEAM ];

  if( buildable != BA_NONE &&
      ( ( 1 << ent->client->ps.weapon ) & BG_Buildable( buildable )->buildWeapon ) &&
      BG_BuildableIsAllowed( buildable ) &&
      ( ( team == TEAM_ALIENS && BG_BuildableAllowedInStage( buildable, g_alienStage.integer ) ) ||
        ( team == TEAM_HUMANS && BG_BuildableAllowedInStage( buildable, g_humanStage.integer ) ) ) )
  {
    dynMenu_t err;
    dist = BG_Class( ent->client->ps.stats[ STAT_CLASS ] )->buildDist;

    ent->client->ps.stats[ STAT_BUILDABLE ] = BA_NONE;

    //these are the errors displayed when the builder first selects something to use
    switch( G_CanBuild( ent, buildable, dist, origin, normal ) )
    {
      // can place right away, set the blueprint and the valid togglebit
      case IBE_NONE:
      case IBE_TNODEWARN:
      case IBE_RPTNOREAC:
      case IBE_RPTPOWERHERE:
      case IBE_SPWNWARN:
        err = MN_NONE;
        // we OR-in the selected builable later
        ent->client->ps.stats[ STAT_BUILDABLE ] = SB_VALID_TOGGLEBIT;
        break;

      // can't place yet but maybe soon: start with valid togglebit off
      case IBE_NORMAL:
      case IBE_NOCREEP:
      case IBE_NOROOM:
      case IBE_NOOVERMIND:
      case IBE_NOPOWERHERE:
	  case IBE_BLOCKEDBYENEMY:
      case IBE_GTHRBLOCKED: 
      case IBE_WSD_INBASE:
        err = MN_NONE;
        break;

      // more serious errors just pop a menu
      case IBE_NOALIENBP:
        err = MN_A_NOBP;
        break;

      case IBE_ONEOVERMIND:
        err = MN_A_ONEOVERMIND;
        break;

      case IBE_ONEREACTOR:
        err = MN_H_ONEREACTOR;
        break;

	  case IBE_COCOON:
        err = MN_A_COCOON;
        break;
		
      case IBE_NOHUMANBP:
        err = MN_H_NOBP;
        break;

      case IBE_NODCC:
        err = MN_H_NODCC;
        break;

      case IBE_PERMISSION:
        err = MN_B_CANNOT;
        break;

      case IBE_LASTSPAWN:
        err = MN_B_LASTSPAWN;
        break;

      case IBE_WSD_REFSCOLS:
        err = MN_B_WSD_REFSCOLS;
        break;

      default:
        err = -1; // stop uninitialised warning
        break;
    }

    if( err == MN_NONE || ent->client->pers.disableBlueprintErrors )
      ent->client->ps.stats[ STAT_BUILDABLE ] |= buildable;
    else
      G_TriggerMenu( ent->client->ps.clientNum, err );
  }
  else
    G_TriggerMenu( ent->client->ps.clientNum, MN_B_CANNOT );
}

/*
=================
Cmd_Reload_f
=================
*/
void Cmd_Reload_f( gentity_t *ent )
{
  playerState_t *ps = &ent->client->ps;
  int ammo;

  // weapon doesn't ever need reloading
  if( BG_Weapon( ps->weapon )->infiniteAmmo )
    return;

  if( ps->clips <= 0 )
    return;

  if( BG_Weapon( ps->weapon )->usesEnergy &&
      BG_InventoryContainsUpgrade( UP_BATTPACK, ps->stats ) )
    ammo = BG_Weapon( ps->weapon )->maxAmmo * BATTPACK_MODIFIER;
  else
    ammo = BG_Weapon( ps->weapon )->maxAmmo;

  // don't reload when full
  if( ps->ammo >= ammo )
    return;

  // the animation, ammo refilling etc. is handled by PM_Weapon
  if( ent->client->ps.weaponstate != WEAPON_RELOADING )
    ent->client->ps.pm_flags |= PMF_WEAPON_RELOAD;
}

/*
=================
G_StopFromFollowing

stops any other clients from following this one
called when a player leaves a team or dies
=================
*/
void G_StopFromFollowing( gentity_t *ent )
{
  int i;

  for( i = 0; i < level.maxclients; i++ )
  {
    if( level.clients[ i ].sess.spectatorState == SPECTATOR_FOLLOW &&
        level.clients[ i ].sess.spectatorClient == ent->client->ps.clientNum )
    {
      if( !G_FollowNewClient( &g_entities[ i ], 1 ) )
        G_StopFollowing( &g_entities[ i ] );
    }
  }
}

/*
=================
G_StopFollowing

If the client being followed leaves the game, or you just want to drop
to free floating spectator mode
=================
*/
void G_StopFollowing( gentity_t *ent )
{
  ent->client->ps.stats[ STAT_TEAM ] = ent->client->pers.teamSelection;

  if( ent->client->pers.teamSelection == TEAM_NONE )
  {
    ent->client->sess.spectatorState =
      ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_FREE;
  }
  else
  {
    vec3_t spawn_origin, spawn_angles;

    ent->client->sess.spectatorState =
      ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_LOCKED;

    if( ent->client->pers.teamSelection == TEAM_ALIENS )
      G_SelectAlienLockSpawnPoint( spawn_origin, spawn_angles );
    else if( ent->client->pers.teamSelection == TEAM_HUMANS )
      G_SelectHumanLockSpawnPoint( spawn_origin, spawn_angles );

    G_SetOrigin( ent, spawn_origin );
    VectorCopy( spawn_origin, ent->client->ps.origin );
    G_SetClientViewAngle( ent, spawn_angles );
  }
  ent->client->sess.spectatorClient = -1;
  ent->client->ps.pm_flags &= ~PMF_FOLLOW;
  ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
  ent->client->ps.stats[ STAT_STATE ] = 0;
  ent->client->ps.stats[ STAT_VIEWLOCK ] = 0;
  ent->client->ps.eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );
  ent->client->ps.viewangles[ PITCH ] = 0.0f;
  ent->client->ps.clientNum = ent - g_entities;
  ent->client->ps.persistant[ PERS_CREDIT ] = ent->client->pers.credit;

  CalculateRanks( );
}

/*
=================
G_FollowLockView

Client is still following a player, but that player has gone to spectator
mode and cannot be followed for the moment
=================
*/
void G_FollowLockView( gentity_t *ent )
{
  vec3_t spawn_origin, spawn_angles;
  int clientNum;

  clientNum = ent->client->sess.spectatorClient;
  ent->client->sess.spectatorState =
    ent->client->ps.persistant[ PERS_SPECSTATE ] = SPECTATOR_FOLLOW;
  ent->client->ps.clientNum = clientNum;
  ent->client->ps.pm_flags &= ~PMF_FOLLOW;
  ent->client->ps.stats[ STAT_TEAM ] = ent->client->pers.teamSelection;
  ent->client->ps.stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
  ent->client->ps.stats[ STAT_VIEWLOCK ] = 0;
  ent->client->ps.eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );
  ent->client->ps.eFlags ^= EF_TELEPORT_BIT;
  ent->client->ps.viewangles[ PITCH ] = 0.0f;

  // Put the view at the team spectator lock position
  if( level.clients[ clientNum ].pers.teamSelection == TEAM_ALIENS )
    G_SelectAlienLockSpawnPoint( spawn_origin, spawn_angles );
  else if( level.clients[ clientNum ].pers.teamSelection == TEAM_HUMANS )
    G_SelectHumanLockSpawnPoint( spawn_origin, spawn_angles );

  G_SetOrigin( ent, spawn_origin );
  VectorCopy( spawn_origin, ent->client->ps.origin );
  G_SetClientViewAngle( ent, spawn_angles );
}

/*
=================
G_FollowNewClient

This was a really nice, elegant function. Then I fucked it up.
=================
*/
qboolean G_FollowNewClient( gentity_t *ent, int dir )
{
  int       clientnum = ent->client->sess.spectatorClient;
  int       original = clientnum;
  qboolean  selectAny = qfalse;

  if( dir > 1 )
    dir = 1;
  else if( dir < -1 )
    dir = -1;
  else if( dir == 0 )
    return qtrue;

  if( ent->client->sess.spectatorState == SPECTATOR_NOT )
    return qfalse;

  // select any if no target exists
  if( clientnum < 0 || clientnum >= level.maxclients )
  {
    clientnum = original = 0;
    selectAny = qtrue;
  }

  do
  {
    clientnum += dir;

    if( clientnum >= level.maxclients )
      clientnum = 0;

    if( clientnum < 0 )
      clientnum = level.maxclients - 1;

    // can't follow self
    if( &g_entities[ clientnum ] == ent )
      continue;

    // avoid selecting existing follow target
    if( clientnum == original && !selectAny )
      continue; //effectively break;

    // can only follow connected clients
    if( level.clients[ clientnum ].pers.connected != CON_CONNECTED )
      continue;

    // can't follow a spectator
    if( level.clients[ clientnum ].pers.teamSelection == TEAM_NONE )
      continue;

    // if stickyspec is disabled, can't follow someone in queue either
    if( !ent->client->pers.stickySpec &&
        level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT )
      continue;

    // can only follow teammates when dead and on a team
    if( ent->client->pers.teamSelection != TEAM_NONE &&
        ( level.clients[ clientnum ].pers.teamSelection !=
          ent->client->pers.teamSelection ) )
      continue;

    // this is good, we can use it
    ent->client->sess.spectatorClient = clientnum;
    ent->client->sess.spectatorState = SPECTATOR_FOLLOW;

    // if this client is in the spawn queue, we need to do something special
    if( level.clients[ clientnum ].sess.spectatorState != SPECTATOR_NOT )
      G_FollowLockView( ent );

    return qtrue;

  } while( clientnum != original );

  return qfalse;
}

/*
=================
G_ToggleFollow
=================
*/
void G_ToggleFollow( gentity_t *ent )
{
  if( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
    G_StopFollowing( ent );
  else
    G_FollowNewClient( ent, 1 );
}

/*
=================
Cmd_Follow_f
=================
*/
void Cmd_Follow_f( gentity_t *ent )
{
  int   i;
  char  arg[ MAX_NAME_LENGTH ];

  // won't work unless spectating
  if( ent->client->sess.spectatorState == SPECTATOR_NOT )
    return;

  if( trap_Argc( ) != 2 )
  {
    G_ToggleFollow( ent );
  }
  else
  {
    char err[ MAX_STRING_CHARS ];
    trap_Argv( 1, arg, sizeof( arg ) );

    i = G_ClientNumberFromString( arg, err, sizeof( err ) );

    if( i == -1 )
    {
      trap_SendServerCommand( ent - g_entities,
        va( "print \"follow: %s\"", err ) );
      return;
    }

    // can't follow self
    if( &level.clients[ i ] == ent->client )
      return;

    // can't follow another spectator if sticky spec is off
    if( !ent->client->pers.stickySpec &&
        level.clients[ i ].sess.spectatorState != SPECTATOR_NOT )
      return;

    // if not on team spectator, you can only follow teammates
    if( ent->client->pers.teamSelection != TEAM_NONE &&
        ( level.clients[ i ].pers.teamSelection !=
          ent->client->pers.teamSelection ) )
      return;

    ent->client->sess.spectatorState = SPECTATOR_FOLLOW;
    ent->client->sess.spectatorClient = i;
  }
}

/*
=================
Cmd_FollowCycle_f
=================
*/
void Cmd_FollowCycle_f( gentity_t *ent )
{
  char args[ 11 ];
  int  dir = 1;

  trap_Argv( 0, args, sizeof( args ) );
  if( Q_stricmp( args, "followprev" ) == 0 )
    dir = -1;

  // won't work unless spectating
  if( ent->client->sess.spectatorState == SPECTATOR_NOT )
    return;

  G_FollowNewClient( ent, dir );
}

static void Cmd_Ignore_f( gentity_t *ent )
{
  int pids[ MAX_CLIENTS ];
  char name[ MAX_NAME_LENGTH ];
  char cmd[ 9 ];
  int matches = 0;
  int i;
  qboolean ignore = qfalse;

  trap_Argv( 0, cmd, sizeof( cmd ) );
  if( Q_stricmp( cmd, "ignore" ) == 0 )
    ignore = qtrue;

  if( trap_Argc() < 2 )
  {
    trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
      "usage: %s [clientNum | partial name match]\n\"", cmd ) );
    return;
  }

  Q_strncpyz( name, ConcatArgs( 1 ), sizeof( name ) );
  matches = G_ClientNumbersFromString( name, pids, MAX_CLIENTS );
  if( matches < 1 )
  {
    trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
      "%s: no clients match the name '%s'\n\"", cmd, name ) );
    return;
  }

  for( i = 0; i < matches; i++ )
  {
    if( ignore )
    {
      if( !Com_ClientListContains( &ent->client->sess.ignoreList, pids[ i ] ) )
      {
        Com_ClientListAdd( &ent->client->sess.ignoreList, pids[ i ] );
        ClientUserinfoChanged( ent->client->ps.clientNum, qfalse );
        trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
          "ignore: added %s^7 to your ignore list\n\"",
          level.clients[ pids[ i ] ].pers.netname ) );
      }
      else
      {
        trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
          "ignore: %s^7 is already on your ignore list\n\"",
          level.clients[ pids[ i ] ].pers.netname ) );
      }
    }
    else
    {
      if( Com_ClientListContains( &ent->client->sess.ignoreList, pids[ i ] ) )
      {
        Com_ClientListRemove( &ent->client->sess.ignoreList, pids[ i ] );
        ClientUserinfoChanged( ent->client->ps.clientNum, qfalse );
        trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
          "unignore: removed %s^7 from your ignore list\n\"",
          level.clients[ pids[ i ] ].pers.netname ) );
      }
      else
      {
        trap_SendServerCommand( ent-g_entities, va( "print \"[skipnotify]"
          "unignore: %s^7 is not on your ignore list\n\"",
          level.clients[ pids[ i ] ].pers.netname ) );
      }
    }
  }
}

/*
=================
Cmd_ListMaps_f

List all maps on the server
=================
*/

static int SortMaps( const void *a, const void *b )
{
  return strcmp( *(char **)a, *(char **)b );
}

#define MAX_MAPLIST_MAPS 256
#define MAX_MAPLIST_ROWS 55
void Cmd_ListMaps_f( gentity_t *ent )
{
  char search[ 16 ] = {""};
  char fileList[ 4096 ] = {""};
  char *fileSort[ MAX_MAPLIST_MAPS ];
  char *filePtr, *p;
  int  numFiles;
  int  fileLen = 0;
  int  shown = 0;
  int  count = 0;
  int  page = 0;
  int  pages;
  int  row, rows;
  int  start, i, j;

  if( trap_Argc( ) > 1 )
  {
    trap_Argv( 1, search, sizeof( search ) );
    for( p = search; ( *p ) && isdigit( *p ); p++ );
    if( !( *p ) )
    {
      page = atoi( search );
      search[ 0 ] = '\0';
    }
    else if( trap_Argc( ) > 2 )
    {
      char lp[ 8 ];
      trap_Argv( 2, lp, sizeof( lp ) );
      page = atoi( lp );
    }

    if( page > 0 )
      page--;
    else if( page < 0 )
      page = 0;
  }

  numFiles = trap_FS_GetFileList( "maps/", ".bsp",
                                  fileList, sizeof( fileList ) );
  filePtr = fileList;
  for( i = 0; i < numFiles && count < MAX_MAPLIST_MAPS; i++, filePtr += fileLen + 1 )
  {
    fileLen = strlen( filePtr );
    if ( fileLen < 5 )
      continue;

    filePtr[ fileLen - 4 ] = '\0';

    if( search[ 0 ] && !strstr( filePtr, search ) )
      continue;

    fileSort[ count ] = filePtr;
    count++;
  }
  qsort( fileSort, count, sizeof( fileSort[ 0 ] ), SortMaps );

  rows = ( count + 2 ) / 3;
  pages = MAX( 1, ( rows + MAX_MAPLIST_ROWS - 1 ) / MAX_MAPLIST_ROWS );
  if( page >= pages )
    page = pages - 1;

  start = page * MAX_MAPLIST_ROWS * 3;
  if( count < start + ( 3 * MAX_MAPLIST_ROWS ) )
    rows = ( count - start + 2 ) / 3;
  else
    rows = MAX_MAPLIST_ROWS;
	
  ADMBP_begin( );
  for( row = 0; row < rows; row++ )
  {
    for( i = start + row, j = 0; i < count && j < 3; i += rows, j++ )
    {
      ADMBP( va( "^5| %-20s", fileSort[ i ] ) );
      shown++;
    }
    ADMBP( "\n" );
  }
  if ( search[ 0 ] )
    ADMBP( va( "^5listmaps: ^5found %d maps matching '%s^7'", count, search ) );
  else
    ADMBP( va( "^5listmaps: ^5listing %d of %d maps", shown, count ) );
  if( pages > 1 )
    ADMBP( va( ", page %d of %d", page + 1, pages ) );
  if( page + 1 < pages )
    ADMBP( va( ", use 'listmaps %s%s%d' to see more",
               search, ( search[ 0 ] ) ? " ": "", page + 2 ) );
  ADMBP( ".\n" );
  ADMBP_end( );
}
/*
=================
Cmd_ListEmoticons_f

List all emoticons on the server
=================
*/

static int SortEmoticons( const void *a, const void *b )
{
  return strcmp( *(char **)a, *(char **)b );
}

void Cmd_ListEmoticons_f( gentity_t *ent )
{
  char search[ 16 ] = {""};
  char fileList[ 4096 ] = {""};
  char *fileSort[ 80 ];
  char *filePtr, *p;
  int  numFiles;
  int  fileLen = 0;
  int  shown = 0;
  int  count = 0;
  int  page = 0;
  int  pages;
  int  row, rows;
  int  start, i, j;

  if( trap_Argc( ) > 1 )
  {
    trap_Argv( 1, search, sizeof( search ) );
    for( p = search; ( *p ) && isdigit( *p ); p++ );
    if( !( *p ) )
    {
      page = atoi( search );
      search[ 0 ] = '\0';
    }
    else if( trap_Argc( ) > 2 )
    {
      char lp[ 8 ];
      trap_Argv( 2, lp, sizeof( lp ) );
      page = atoi( lp );
    }

    if( page > 0 )
      page--;
    else if( page < 0 )
      page = 0;
  }

  numFiles = trap_FS_GetFileList( "emoticons/", ".tga",
                                  fileList, sizeof( fileList ) );
  filePtr = fileList;
  for( i = 0; i < numFiles && count < 80; i++, filePtr += fileLen + 1 )
  {
    fileLen = strlen( filePtr );
    if ( fileLen < 5 )
      continue;

    filePtr[ fileLen - 4 ] = '\0';

    if( search[ 0 ] && !strstr( filePtr, search ) )
      continue;

    fileSort[ count ] = filePtr;
    count++;
  }
  qsort( fileSort, count, sizeof( fileSort[ 0 ] ), SortEmoticons );

  rows = ( count + 2 ) / 3;
  pages = MAX( 1, ( rows + 10 - 1 ) / 10 );
  if( page >= pages )
    page = pages - 1;

  start = page * 10 * 3;
  if( count < start + ( 3 * 10 ) )
    rows = ( count - start + 2 ) / 3;
  else
    rows = 10;

  ADMBP_begin( );
  for( row = 0; row < rows; row++ )
  {
    for( i = start + row, j = 0; i < count && j < 3; i += rows, j++ )
    {
      ADMBP( va( "^7 %-20s", fileSort[ i ] ) );
      shown++;
    }
    ADMBP( "\n" );
  }
  if ( search[ 0 ] )
    ADMBP( va( "^3listemoticons: ^7found %d emoticons matching '%s^7'", count, search ) );
  else
    ADMBP( va( "^3listemoticons: ^7listing %d of %d emoticons", shown, count ) );
  if( pages > 1 )
    ADMBP( va( ", page %d of %d", page + 1, pages ) );
  if( page + 1 < pages )
    ADMBP( va( ", use 'listemoticons %s%s%d' to see more",
               search, ( search[ 0 ] ) ? " ": "", page + 2 ) );
  ADMBP( ".\n" );
  ADMBP_end( );
}
/*
=================
Cmd_MapRotation_f

Print current map rotation
=================
*/
void Cmd_MapRotation_f( gentity_t *ent )
{
  G_PrintCurrentRotation( ent, "^3rotation:" );
}

#define MAX_MAPLOGS 5

typedef struct {
  int flag;
  char *description;
} mapLogResult_t;

static mapLogResult_t maplog_table[] = {
  { 't', "^7tie" },
  { 'a', "^1Alien win" },
  { 'A', "^1Alien win ^7/ Humans admitted defeat" },
  { 'h', "^5Human win" },
  { 'H', "^5Human win ^7/ Aliens admitted defeat" },
  { 'd', "^2draw vote" },
  { 'm', "^2map vote" },
  { 'r', "^2restart vote" },
  { 'M', "^6admin changed map" },
  { 'N', "^6admin loaded next map" },
  { 'R', "^6admin restarted map" },
  { '\0', "" }
};

void G_MapLog_NewMap( void )
{
  char maplog[ MAX_CVAR_VALUE_STRING ];
  char map[ MAX_QPATH ];
  char *ptr;
  int  count = 0;

  trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );
  Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
  ptr = maplog;
  while( *ptr && count < MAX_MAPLOGS )
  {
    while( *ptr && *ptr != ' ' )
      ptr++;
    count++;
    if( count == MAX_MAPLOGS )
      *ptr = '\0';
    if( *ptr == ' ' )
      ptr++;
  }

  trap_Cvar_Set( "g_mapLog",
    va( "%s%s%s",
        map, 
        ( maplog[ 0 ] ) ? " " : "",
        maplog ) );
}

void G_MapLog_Result( char result )
{
  static int lastTime = 0;
  char   maplog[ MAX_CVAR_VALUE_STRING ];
  int    t;

  // there is a chance is called more than once per frame
  if( level.time == lastTime )
    return;
  lastTime = level.time;

  // check for earlier result
  if( g_mapLog.string[ 0 ] && g_mapLog.string[ 1 ] == ';' )
    return;

  if( level.surrenderTeam != TEAM_NONE )
  {
    if( result == 'a' && level.surrenderTeam == TEAM_HUMANS )
      result = 'A';
    if( result == 'h' && level.surrenderTeam == TEAM_ALIENS )
      result = 'H';
  }

  t = ( level.time - level.startTime ) / 1000;
  Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
  trap_Cvar_Set( "g_mapLog",
    va( "%c;%d:%02d;%s", result, t / 60, t % 60, maplog ) );
}

/*
=================
Cmd_MapLog_f

Print recent map results
=================
*/
void Cmd_MapLog_f( gentity_t *ent )
{
  char maplog[ MAX_CVAR_VALUE_STRING ];
  char *ptr;
  int  i;

  Q_strncpyz( maplog, g_mapLog.string, sizeof( maplog ) );
  ptr = maplog;

  ADMBP_begin( );
  ADMBP( "^3maplog: ^7recent map results, newest first\n" );
  while( *ptr )
  {
    char *clock = "  -:--";
    char *result = "^1unknown";
    char *end = ptr;

    while( *end && *end != ' ' )
      end++;
    if( *end == ' ' )
    {
      *end = '\0';
      end++;
    }

    if( ptr[ 0 ] && ptr[ 1 ] == ';' )
    {
      for( i = 0; maplog_table[ i ].flag; i++ )
      {
        if( maplog_table[ i ].flag == *ptr )
        {
          result = maplog_table[ i ].description;
          break;
        }
      }
      ptr += 2;
      clock = ptr;
      while( *ptr && *ptr != ';' )
        ptr++;
      if( *ptr == ';' )
      {
        *ptr = '\0';
        ptr++;
      }
    }
    else if( ptr == maplog )
    {
      result = "^7current map";
    }

    ADMBP( va( "  ^%s%-20s %6s %s^7\n",
               ( ptr == maplog ) ? "2" : "7",
               ptr, clock, result ) );
    ptr = end;
  }
  ADMBP_end( );
}

/*
=================
Cmd_Test_f
=================
*/
void Cmd_Test_f( gentity_t *humanPlayer )
{
}

/*
=================
Cmd_Damage_f

Deals damage to you (for testing), arguments: [damage] [dx] [dy] [dz]
The dx/dy arguments describe the damage point's offset from the entity origin
=================
*/
void Cmd_Damage_f( gentity_t *ent )
{
  vec3_t point;
  char arg[ 16 ];
  float dx = 0.0f, dy = 0.0f, dz = 100.0f;
  int damage = 100;
  qboolean nonloc = qtrue;

  if( trap_Argc() > 1 )
  {
    trap_Argv( 1, arg, sizeof( arg ) );
    damage = atoi( arg );
  }
  if( trap_Argc() > 4 )
  {
    trap_Argv( 2, arg, sizeof( arg ) );
    dx = atof( arg );
    trap_Argv( 3, arg, sizeof( arg ) );
    dy = atof( arg );
    trap_Argv( 4, arg, sizeof( arg ) );
    dz = atof( arg );
    nonloc = qfalse;
  }
  VectorCopy( ent->s.origin, point );
  point[ 0 ] += dx;
  point[ 1 ] += dy;
  point[ 2 ] += dz;
  G_Damage( ent, NULL, NULL, NULL, point, damage,
            ( nonloc ? DAMAGE_NO_LOCDAMAGE : 0 ), MOD_TARGET_LASER );
}

/*
==================
G_FloodLimited

Determine whether a user is flood limited, and adjust their flood demerits
Print them a warning message if they are over the limit
Return is time in msec until the user can speak again
==================
*/
int G_FloodLimited( gentity_t *ent )
{
  int deltatime, ms;

  if( g_floodMinTime.integer <= 0 )
    return 0;

  // handles !ent
  if( G_admin_permission( ent, ADMF_NOCENSORFLOOD ) )
    return 0;

  deltatime = level.time - ent->client->pers.floodTime;

  ent->client->pers.floodDemerits += g_floodMinTime.integer - deltatime;
  if( ent->client->pers.floodDemerits < 0 )
    ent->client->pers.floodDemerits = 0;
  ent->client->pers.floodTime = level.time;

  ms = ent->client->pers.floodDemerits - g_floodMaxDemerits.integer;
  if( ms <= 0 )
    return 0;
  trap_SendServerCommand( ent - g_entities, va( "print \"You are flooding: "
                          "please wait %d second%s before trying again\n",
                          ( ms + 999 ) / 1000, ( ms > 1000 ) ? "s" : "" ) );
  return ms;
}

commands_t cmds[ ] = {
  { "a", CMD_MESSAGE|CMD_INTERMISSION, Cmd_AdminMessage_f },
  { "build", CMD_TEAM|CMD_LIVING, Cmd_Build_f },
  { "buy", CMD_HUMAN|CMD_LIVING, Cmd_Buy_f },
  { "callteamvote", CMD_MESSAGE|CMD_TEAM, Cmd_CallVote_f },
  { "callvote", CMD_MESSAGE, Cmd_CallVote_f },
  { "class", CMD_TEAM, Cmd_Class_f },
  { "damage", CMD_CHEAT|CMD_LIVING, Cmd_Damage_f },
  { "deconstruct", CMD_TEAM|CMD_LIVING, Cmd_Destroy_f },
  { "destroy", CMD_CHEAT|CMD_TEAM|CMD_LIVING, Cmd_Destroy_f },
  { "follow", CMD_SPEC, Cmd_Follow_f },
  { "follownext", CMD_SPEC, Cmd_FollowCycle_f },
  { "followprev", CMD_SPEC, Cmd_FollowCycle_f },
  { "give", CMD_CHEAT|CMD_TEAM|CMD_LIVING, Cmd_Give_f },
  { "god", CMD_CHEAT|CMD_TEAM|CMD_LIVING, Cmd_God_f },
  { "ignore", 0, Cmd_Ignore_f },
  { "itemact", CMD_HUMAN|CMD_LIVING, Cmd_ActivateItem_f },
  { "itemdeact", CMD_HUMAN|CMD_LIVING, Cmd_DeActivateItem_f },
  { "itemtoggle", CMD_HUMAN|CMD_LIVING, Cmd_ToggleItem_f },
  { "kill", CMD_TEAM|CMD_LIVING, Cmd_Kill_f },
  { "levelshot", CMD_CHEAT, Cmd_LevelShot_f },
  { "listemoticons", CMD_MESSAGE|CMD_INTERMISSION, Cmd_ListEmoticons_f },
  { "listmaps", CMD_MESSAGE|CMD_INTERMISSION, Cmd_ListMaps_f },
  { "maplog", CMD_MESSAGE|CMD_INTERMISSION, Cmd_MapLog_f },
  { "myscore", 0, Cmd_MyScore_f },
  { "noclip", CMD_CHEAT_TEAM, Cmd_Noclip_f },
  { "notarget", CMD_CHEAT|CMD_TEAM|CMD_LIVING, Cmd_Notarget_f },
  { "reload", CMD_HUMAN|CMD_LIVING, Cmd_Reload_f },
  { "rotation", CMD_MESSAGE|CMD_INTERMISSION, Cmd_MapRotation_f },
  { "score", CMD_INTERMISSION, ScoreboardMessage },
  { "sell", CMD_HUMAN|CMD_LIVING, Cmd_Sell_f },
  { "setviewpos", CMD_CHEAT_TEAM, Cmd_SetViewpos_f },
  { "team", 0, Cmd_Team_f },
  { "teamvote", CMD_TEAM, Cmd_Vote_f },
  { "test", CMD_CHEAT, Cmd_Test_f },
  { "unignore", 0, Cmd_Ignore_f },
  { "vote", 0, Cmd_Vote_f },
  { "where", 0, Cmd_Where_f }
};
static size_t numCmds = sizeof( cmds ) / sizeof( cmds[ 0 ] );

/*
=================
ClientCommand
=================
*/
void ClientCommand( int clientNum )
{
  gentity_t  *ent;
  char       cmd[ MAX_TOKEN_CHARS ];
  commands_t *command;

  ent = g_entities + clientNum;
  if( !ent->client || ent->client->pers.connected != CON_CONNECTED )
    return;   // not fully in game yet

  trap_Argv( 0, cmd, sizeof( cmd ) );

  command = bsearch( cmd, cmds, numCmds, sizeof( cmds[ 0 ] ), cmdcmp );

  if( !command )
  {
    if( !G_admin_cmd_check( ent ) )
      trap_SendServerCommand( clientNum,
        va( "print \"Unknown command %s\n\"", cmd ) );
    return;
  }

  // do tests here to reduce the amount of repeated code

  if( !( command->cmdFlags & CMD_INTERMISSION ) &&
      ( level.intermissiontime || level.pausedTime ) )
    return;

  if( command->cmdFlags & CMD_CHEAT && !g_cheats.integer )
  {
    G_TriggerMenu( clientNum, MN_CMD_CHEAT );
    return;
  }

  if( command->cmdFlags & CMD_MESSAGE && ( ent->client->pers.namelog->muted ||
      G_FloodLimited( ent ) ) )
    return;

  if( command->cmdFlags & CMD_TEAM &&
      ent->client->pers.teamSelection == TEAM_NONE )
  {
    G_TriggerMenu( clientNum, MN_CMD_TEAM );
    return;
  }

  if( command->cmdFlags & CMD_CHEAT_TEAM && !g_cheats.integer &&
      ent->client->pers.teamSelection != TEAM_NONE )
  {
    G_TriggerMenu( clientNum, MN_CMD_CHEAT_TEAM );
    return;
  }

  if( command->cmdFlags & CMD_SPEC &&
      ent->client->sess.spectatorState == SPECTATOR_NOT )
  {
    G_TriggerMenu( clientNum, MN_CMD_SPEC );
    return;
  }

  if( command->cmdFlags & CMD_ALIEN &&
      ent->client->pers.teamSelection != TEAM_ALIENS )
  {
    G_TriggerMenu( clientNum, MN_CMD_ALIEN );
    return;
  }

  if( command->cmdFlags & CMD_HUMAN &&
      ent->client->pers.teamSelection != TEAM_HUMANS )
  {
    G_TriggerMenu( clientNum, MN_CMD_HUMAN );
    return;
  }

  if( command->cmdFlags & CMD_LIVING &&
    ( ent->client->ps.stats[ STAT_HEALTH ] <= 0 ||
      ent->client->sess.spectatorState != SPECTATOR_NOT ) )
  {
    G_TriggerMenu( clientNum, MN_CMD_LIVING );
    return;
  }

  command->cmdHandler( ent );
}

void G_ListCommands( gentity_t *ent )
{
  int   i;
  char  out[ MAX_STRING_CHARS ] = "";
  int   len, outlen;

  outlen = 0;

  for( i = 0; i < numCmds; i++ )
  {
    len = strlen( cmds[ i ].cmdName ) + 1;
    if( len + outlen >= sizeof( out ) - 1 )
    {
      trap_SendServerCommand( ent - g_entities, va( "cmds%s\n", out ) );
      outlen = 0;
    }

    strcpy( out + outlen, va( " %s", cmds[ i ].cmdName ) );
    outlen += len;
  }

  trap_SendServerCommand( ent - g_entities, va( "cmds%s\n", out ) );
  G_admin_cmdlist( ent );
}

void G_DecolorString( char *in, char *out, int len )
{
  qboolean decolor = qtrue;

  len--;

  while( *in && len > 0 ) {
    if( *in == DECOLOR_OFF || *in == DECOLOR_ON )
    {
      decolor = ( *in == DECOLOR_ON );
      in++;
      continue;
    }
    if( Q_IsColorString( in ) && decolor ) {
      in += 2;
      continue;
    }
    *out++ = *in++;
    len--;
  }
  *out = '\0';
}

void G_UnEscapeString( char *in, char *out, int len )
{
  len--;

  while( *in && len > 0 )
  {
    if( *in >= ' ' || *in == '\n' )
    {
      *out++ = *in;
      len--;
    }
    in++;
  }
  *out = '\0';
}

void Cmd_MyScore_f( gentity_t *ent )
{
  g_admin_level_t *l,*n;
  if ( !ent || !ent->client->pers.admin ) {
    ADMP( "This command is only available for registered players.\n" );
    return;
  }
  l = G_admin_find_level_for_score( ent->client->pers.admin->score );
  if (( n = G_admin_level_next( G_admin_level( l->level ) ) )) {
    ADMP( va( "^7Level %d (%s^7) total score earned: %d next level: %d\n", 
      l->level, G_admin_level( l->level )->name, ent->client->pers.admin->score, n->score ) );
  } else {
    ADMP( va( "^7Level %d (%s^7) total score earned: %d (max level)\n", 
      l->level, G_admin_level( l->level )->name, ent->client->pers.admin->score ) );
  }
}

/*
=================
Cmd_AdminMessage_f

Send a message to all active admins
=================
*/
void Cmd_AdminMessage_f( gentity_t *ent )
{
  // Check permissions and add the appropriate user [prefix]
  if( !G_admin_permission( ent, ADMF_ADMINCHAT ) )
  {
    if( !g_publicAdminMessages.integer )
    {
      ADMP( "Sorry, but use of /a by non-admins has been disabled.\n" );
      return;
    }
    else
    {
      ADMP( "Your message has been sent to any available admins "
            "and to the server logs.\n" );
    }
  }

  if( trap_Argc( ) < 2 )
  {
    ADMP( "usage: a [message]\n" );
    return;
  }

  G_AdminMessage( ent, ConcatArgs( 1 ) );
}

