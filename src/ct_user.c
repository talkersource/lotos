/*****************************************************************************
              Funkcie OS Star v1.1.1 pre userovu osobnu potrebu
            Copyright (C) Pavol Hluchy - posledny update: 5.10.2000
          osstar@star.sjf.stuba.sk  |  http://star.sjf.stuba.sk/osstar
 *****************************************************************************/

#include <stdio.h>
#include <time.h>
#include <string.h>

#include "define.h"
#include "ur_obj.h"
#include "rm_obj.h"
#ifdef NETLINKS
	#include "nl_obj.h"
#endif
#include "sys_obj.h"
#include "syspp_obj.h"
#include "ct_user.h"
#include "comvals.h"
#include "setval.h"
#include "ignval.h"

/* */
UR_OBJECT create_user(void);
UR_OBJECT get_user(char *name);
UR_OBJECT get_user_name(UR_OBJECT user, char *i_name);
char * colour_com_strip(char *str);
char * remove_first(char *inpstr);
char * censor_swear_words(char *has_swears);


/*** Set user description ***/
void set_desc(UR_OBJECT user, char *inpstr)
{
if (word_count<2) {
  vwrite_user(user,"Your current description is: %s\n",user->desc);
  return;
  }
if (strstr(word[1],"(CLONE)")) {
  write_user(user,"You cannot have that description.\n");
  return;
  }
if (strlen(inpstr)>USER_DESC_LEN) {
  write_user(user,"Description too long.\n");
  return;
  }
strcpy(user->desc,inpstr);
write_user(user,"Description set.\n");
/* check to see if user should be promoted */
if (amsys->auto_promote) check_autopromote(user,2);
}


/*** Set in and out phrases ***/
void set_iophrase(UR_OBJECT user, char *inpstr)
{
if (strlen(inpstr)>PHRASE_LEN) {
  write_user(user,"Phrase too long.\n");  return;
  }
if (com_num==INPHRASE) {
  if (word_count<2) {
    vwrite_user(user,"Your current in phrase is: %s\n",user->in_phrase);
    return;
    }
  strcpy(user->in_phrase,inpstr);
  write_user(user,"In phrase set.\n");
  return;
  }
if (word_count<2) {
  vwrite_user(user,"Your current out phrase is: %s\n",user->out_phrase);
  return;
  }
strcpy(user->out_phrase,inpstr);
write_user(user,"Out phrase set.\n");
}


/*** Show some user stats ***/
void status(UR_OBJECT user)
{
	UR_OBJECT u;
	int on, days, hours, mins, hs, newmail;
	char nm[5], *pp;

	if (word_count<2) {
		u=user;
		on=1;
		}
	else {
		if (!(u=get_user(word[1]))) {
			if ((u=create_user())==NULL) {
				vwrite_user(user, "%s: unable to create temporary user session.\n",syserror);
				write_syslog(ERRLOG,0,"Unable to create temporary user session in status().\n");
				return;
				}
			strcpy(u->name,word[1]);
			if (!load_user_details(u)) {
				write_user(user,nosuchuser);
				destruct_user(u);
				destructed=0;
				return;
				}
			on=0;
			}
		else on=1;
		}

	if (user->level<u->level) {
		write_user(user, "Nemozes si pozret status usera s vyssim levelom ako mas ty\n");
		if (!on) {
			destruct_user(u);
			destructed=0;
			}
		return;
		}

	write_user(user, "\n");
	vwrite_user(user, "Status pre %s~RS %s\n",
		u->recap, u->desc);
	if (on) write_user(user, "~OL.-- ~FTSystem info~FW ----~FY(on line)~FW----------------------------------- ~FTUser info ~FW----.\n");
	else write_user(user, "~OL.-- ~FTSystem info~FW ----~FB(off line)~FW---------------------------------- ~FTUser info ~FW----.\n");
	if (on) {
		sprintf(text, "site~RS       : %s:%d", u->site, u->site_port);
		vwrite_user(user, "~OL|~RS ~FG%-79.79s ~OL|\n", text);
		}
	vwrite_user(user, "~OL|~RS ~FGlast site~RS  : %-46.46s~OL|~RS ~FGlevel~RS %s         ~OL|\n",
		u->last_site, user_level[u->level].alias);
	days=u->total_login/86400;
	hours=(u->total_login%86400)/3600;
	mins=(u->total_login%3600)/60;
	hs=(int)(time(0)-u->last_login)/60;
	sprintf(text, "%dd %dh %dm", days, hours, mins);
	if (on)	vwrite_user(user, "~OL|~RS ~FGtotal login~RS: %-20.20s                          ~OL|~RS ~FGonline~RS %-5d min~OL|\n",
		text, hs);
	else vwrite_user(user, "~OL|~RS ~FGtotal login~RS: %-20.20s                          ~OL|~RS                 ~OL|\n",
		text);
	vwrite_user(user, "~OL|~RS ~FGhomepage~RS   : %-46.46s~OL|~RS ~FGgender~RS %-8.8s ~OL|\n",
		(u->homepage[0]!='#')?u->homepage:"nenastavena", sex[u->gender]);
	if (!strcmp(u->email, "#UNSET")) sprintf(text, "~FR~OLnenastaveny~RS");
	else {
		sprintf(text, "%s", u->email);
		if (u->hideemail) strcat(text, " ~FB~OL(SKRYTY)~RS");
		}
	if (u==user
	    || !u->hideemail
	    || (user->level>=ARCH && user->level>u->level)
	    ) {
		hs=colour_com_count(text);
	vwrite_user(user, "~OL|~RS ~FGe-mail~RS     : %-*.*s ~OL|\n", 63+hs*3, 63+hs*3, text);
	}

	if (u==user || user->level>=GOD) {
		write_user(user, "~OL|-- ~FTPrivat info~FW ---------------------------------------------------------------|\n");
		newmail=mail_sizes(u->name, 1);
		if (newmail) sprintf(nm, "%d", newmail);
		else sprintf(nm, "NIE");
		write_user(user, "~OL|~RS ~FGNewMail monitor autoFWD CharEcho wrap Pueblo riadky farby AudioPrompt xterm  ~RS~OL|\n");
		vwrite_user(user, "~OL|~RS   %-3.3s     %-3.3s     %-3.3s     %-3.3s    %-3.3s   %-3.3s     %-2.2d     %-1.1d       %-3.3s      %-3.3s   ~OL|\n",
			nm, offon[u->monitor], noyes2[u->autofwd], noyes2[u->charmode_echo],
			noyes2[u->wrap], noyes2[u->pueblo], u->pager, u->colour,
			noyes2[(u->pueblo_mm && u->pueblo)], offon[u->xterm]);
		sprintf(text, "Killed %d people, and died %d times.  Energy : %d, Bullets : %d",
			u->kills, u->deaths, u->hps, u->bullets);
		vwrite_user(user,"~OL|~RS %-76.76s ~OL|\n", text);
		}

	if (user->level>=GOD && u!=user) {
		write_user(user, "~OL|-- ~FTPhrases~FW -------------------------------------------------------------------|\n");
		vwrite_user(user, "~OL|~RS ~FGin~RS : %-71.71s~RS ~OL|\n", u->in_phrase);
		vwrite_user(user, "~OL|~RS ~FGout~RS: %-71.71s~RS ~OL|\n", u->out_phrase);
		}

	write_user(user, "~OL|-- ~FTIgnore~FW --------------------------------------------------------------------|\n");
	write_user(user, "~OL|~RS   ~FGAll    Shout    Tell    Greet    Fun    Wiz    Pic    Beep    Transport~RS    ~OL|\n");
	vwrite_user(user, "~OL|~RS   %-3.3s     %-3.3s     %-3.3s      %-3.3s     %-3.3s    %-3.3s    %-3.3s    %-3.3s       %-3.3s        ~OL|\n",
		noyes2[u->ignall], noyes2[u->ignshouts], noyes2[u->igntells],
		noyes2[u->igngreets], noyes2[u->ignfuns], noyes2[u->ignwiz],
		noyes2[u->ignpics], noyes2[u->ignbeeps], noyes2[u->igntr]
		);

	if (user->level>=WIZ) {
		write_user(user, "~OL|-- ~FTWiz info~FW ------------------------------------------------------------------|\n");
		vwrite_user(user, "~OL|~RS ~FGICQ~RS     : %-10.10s       ~FGvek~RS     : %-3d      ~FGtot. logins~RS: %-9d         ~OL|\n",
			(u->icq[0]!='#')?u->icq:"unset", u->age, u->logons);
		vwrite_user(user, "~OL|~RS ~FGrestrict~RS: %-14.14s   ~FGbcounter~RS: %-6d   ~FGtcounter~RS   : %-9d         ~OL|\n",
			u->restrict, u->bcount, u->tcount);
		strcpy(text, ctime((time_t *)&u->t_expire));
		if ((pp=strchr(text, '\n'))) pp[0]='\0';
		vwrite_user(user, "~OL|~RS ~FGexpire~RS  : %-3.3s  o %-26.26s   ~FGverified~RS   : %-3.3s               ~OL|\n",
			noyes2[u->expire], text, noyes2[u->mail_verified]);
		}

	write_user(user, "~OL|-- ~FTOther info~FW ----------------------------------------------------------------|\n");
	vwrite_user(user, "~OL|~RS   ~FGJailLev~RS: %-3.3s      ~FGUnJailLev~RS: %-3.3s      ~FGUnMuzzLev~RS: %-3.3s      ~FGprilep~RS %-3.3s       ~OL|\n",
		(u->arrestby)?user_level[u->arrestby].alias:"not",
		user_level[u->unarrest].alias,
		(u->muzzled)?user_level[u->muzzled].alias:"not",
		noyes2[(u->lroom==2)]
		);
	write_user(user, "~OL`------------------------------------------------------------------------------'\n");

	if (!on) {
		destruct_user(u);
		destructed=0;
		}
}


/*** Enter user profile ***/
void enter_profile(UR_OBJECT user, int done_editing)
{
FILE *fp;
char *c,filename[500];

if (!done_editing) {
  write_user(user, profile_edit_header);
  user->misc_op=5;
  editor(user,NULL);
  return;
  }
sprintf(filename,"%s/%s.P", USERPROFILES,user->name);
if (!(fp=fopen(filename,"w"))) {
  vwrite_user(user,"%s: couldn't save your profile.\n",syserror);
  write_syslog(SYSLOG,0,"ERROR: Couldn't open file %s to write in enter_profile().\n",filename);
  return;
  }
c=user->malloc_start;
while(c!=user->malloc_end) putc(*c++,fp);
fclose(fp);
write_user(user,"Profile stored.\n");
}


/*** Examine a user ***/
void examine(UR_OBJECT user)
{
	UR_OBJECT u;
	FILE *fp;
	char filename[500], text2[ARR_SIZE], line[182];
	int on,days,hours,mins,timelen,days2,hours2,mins2,idle,cnt,newmail;
	int prf;

	if (word_count<2) {
		u=user;
		on=1;
		}
	else {
		if (!(u=get_user(word[1]))) {
			if ((u=create_user())==NULL) {
				vwrite_user(user,"%s: nemozem vytvorit docasny user objekt.\n",syserror);
				write_syslog(SYSLOG,0,"ERROR: Unable to create temporary user object in examine().\n");
				return;
				}
			strcpy(u->name,word[1]);
			if (!load_user_details(u)) {
				write_user(user,nosuchuser);   
				destruct_user(u);
				destructed=0;
				return;
				}
			on=0;
			}
		else on=1;
		}

	days=u->total_login/86400;
	hours=(u->total_login%86400)/3600;
	mins=(u->total_login%3600)/60;
	timelen=(int)(time(0) - u->last_login);
	days2=timelen/86400;
	hours2=(timelen%86400)/3600;
	mins2=(timelen%3600)/60;
	prf=strncmp("-noprofile", word[2], strlen(word[2]));
	if (word_count<3) prf=1;

	write_user(user,"\n+----------------------------------------------------------------------------+\n");
	sprintf(text2,"%s~RS %s",u->recap,u->desc);
	cnt=colour_com_count(text2);
	vwrite_user(user,"Name   : %-*.*s~RS Level : %s\n",45+cnt*3,45+cnt*3,text2,user_level[u->level].name);
	if (!on) {
		vwrite_user(user,"Last login : %s",ctime((time_t *)&(u->last_login)));
		vwrite_user(user,"Which was  : %d day%s, %d hour%s, %d minute%s ago\n",days2,PLTEXT_S(days2),hours2,PLTEXT_S(hours2),mins2,PLTEXT_S(mins2));
		vwrite_user(user,"Was on for : %d hours, %d minutes\nTotal login: %d day%s, %d hour%s, %d minute%s\n",
		u->last_login_len/3600,(u->last_login_len%3600)/60,days,PLTEXT_S(days),hours,PLTEXT_S(hours),mins,PLTEXT_S(mins));
		if (user->level>=WIZ) vwrite_user(user,"Last site  : %s\n",u->last_site);
		if ((newmail=mail_sizes(u->name,1))) vwrite_user(user,"%s~RS has unread mail (%d).\n",u->recap,newmail);
		write_user(user,"+----- ~OL~FTProfile~RS --------------------------------------------------------------+\n\n");
		if (prf) {
			sprintf(filename,"%s/%s.P", USERPROFILES,u->name);
			if (!(fp=fopen(filename,"r"))) write_user(user, no_profile_prompt);
			else {
				fgets(line, 81, fp);
				while(!feof(fp)) {
					replace_string(line, "{name}", user->name);
					replace_string(line, "{level}", user_level[user->level].name);
					write_user(user, line);
					fgets(line, 81, fp);
					}
				fclose(fp);
				}
			}
		else write_user(user, "Preskakujem profil\n");
		write_user(user,"+----------------------------------------------------------------------------+\n\n");
		destruct_user(u);
		destructed=0;
		return;
		}
	idle=(int)(time(0) - u->last_input)/60;
	if (u->editing) vwrite_user(user,"Ignoring all: ~FTUsing Line Editor~RS\n");
	else vwrite_user(user,"Ignoring all: %s\n",noyes2[u->ignall]);
	vwrite_user(user,"On since    : %sOn for      : %d hour%s, %d minute%s\n",ctime((time_t *)&u->last_login),hours2,PLTEXT_S(hours2),mins2,PLTEXT_S(mins2));
	if (u->afk) {
		vwrite_user(user,"Idle for    : %d minute%s ~BR(AFK)\n",idle,PLTEXT_S(idle));
		if (u->afk_mesg[0]) vwrite_user(user,"AFK message : %s\n",u->afk_mesg);
		}
	else vwrite_user(user,"Idle for    : %d minute%s\n",idle,PLTEXT_S(idle));
	vwrite_user(user,"Total login : %d day%s, %d hour%s, %d minute%s\n",days,PLTEXT_S(days),hours,PLTEXT_S(hours),mins,PLTEXT_S(mins));
	if (u->socket>=1)
		if (user->level>=WIZ) vwrite_user(user,"Site        : %-40.40s  Port : %d\n",u->site,u->site_port);
#ifdef NETLINKS
		else vwrite_user(user,"Home service: %s\n",u->netlink->service);
#endif
	if ((newmail=mail_sizes(u->name,1))) vwrite_user(user,"%s~RS has unread mail (%d).\n",u->recap,newmail);
	write_user(user,"+----- ~OL~FTProfile~RS --------------------------------------------------------------+\n\n");
	if (prf) {
		sprintf(filename,"%s/%s.P", USERPROFILES,u->name);
		if (!(fp=fopen(filename,"r"))) write_user(user, no_profile_prompt);
		else {
			fgets(line, 81, fp);
			while(!feof(fp)) {
				replace_string(line, "{name}", user->name);
				replace_string(line, "{level}", user_level[user->level].name);
				write_user(user, line);
				fgets(line, 81, fp);
				}
			fclose(fp);
			}
		}
	else write_user(user, "Preskakujem profil\n");
	write_user(user,"+----------------------------------------------------------------------------+\n\n");
	if (user->level<=u->level && user!=u)
		vwrite_user(u, "%s si ta oscannoval%s\n", user->name, gnd_grm(4, user->gender));
}


/*** Change users password. Only GODs and above can change another users 
     password and they do this by specifying the user at the end. When this is 
     done the old password given can be anything, the wiz doesnt have to know it
     in advance. ***/
void change_pass(UR_OBJECT user)
{
UR_OBJECT u;
char *name;

if (word_count<3) {
  if (user->level<GOD) write_usage(user,"passwd <stare heslo> <nove heslo>");
  else write_usage(user,"passwd <stare heslo> <nove heslo> [<user>]");
  return;
  }
if (strlen(word[2])<PASS_MIN_LEN) {
  write_user(user,"New password too short.\n");  return;
  }
if (strlen(word[2])>PASS_LEN) {
  write_user(user,"New password too long.\n");  return;
  }
/* Change own password */
if (word_count==3) {
  if (strcmp((char *)crypt(word[1],crypt_salt),user->pass)) {
    write_user(user,"Old password incorrect.\n");  return;
    }
  if (!strcmp(word[1],word[2])) {
    write_user(user,"Old and new passwords are the same.\n");  return;
    }
  strcpy(user->pass,(char *)crypt(word[2],crypt_salt));
  save_user_details(user,0);
  cls(user);
  write_user(user,"Password changed.\n");
  add_history(user->name,1,"Changed passwords.\n");
  return;
  }
/* Change someone elses */
if (user->level<GOD) {
  write_user(user,"You are not a high enough level to use the user option.\n");  
  return;
  }
word[3][0]=toupper(word[3][0]);
if (!strcmp(word[3],user->name)) {
  /* security feature  - prevents someone coming to a wizes terminal and 
     changing his password since he wont have to know the old one */
  write_user(user,"You cannot change your own password using the <user> option.\n");
  return;
  }
if ((u=get_user(word[3]))) {
#ifdef NETLINKS
  if (u->type==REMOTE_TYPE) {
    write_user(user,"You cannot change the password of a user logged on remotely.\n");
    return;
    }
#endif
  if (u->level>=user->level) {
    write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
    return;
    }
  strcpy(u->pass,(char *)crypt(word[2],crypt_salt));
  cls(user);
  vwrite_user(user,"%s's password has been changed.\n",u->name);
  if (user->vis) name=user->name; else name=invisname;
  vwrite_user(u,"~FR~OLYour password has been changed by %s!\n",name);
  write_syslog(SYSLOG,1,"%s changed %s's password.\n",user->name,u->name);
  sprintf(text,"Forced password change by %s.\n",user->name);
  add_history(u->name,0,text);
  return;
  }
if ((u=create_user())==NULL) {
  vwrite_user(user,"%s: unable to create temporary user object.\n",syserror);
  write_syslog(SYSLOG,0,"ERROR: Unable to create temporary user object in change_pass().\n");
  return;
  }
strcpy(u->name,word[3]);
if (!load_user_details(u)) {
  write_user(user,nosuchuser);   
  destruct_user(u);
  destructed=0;
  return;
  }
if (u->level>=user->level) {
  write_user(user,"You cannot change the password of a user of equal or higher level than yourself.\n");
  destruct_user(u);
  destructed=0;
  return;
  }
strcpy(u->pass,(char *)crypt(word[2],crypt_salt));
save_user_details(u,0);
destruct_user(u);
destructed=0;
cls(user);
vwrite_user(user,"%s's password changed to \"%s\".\n",word[3],word[2]);
sprintf(text,"Forced password change by %s.\n",user->name);
add_history(u->name,0,text);
write_syslog(SYSLOG,1,"%s changed %s's password.\n",user->name,u->name);
}


/*** Set user visible or invisible ***/
void visibility(UR_OBJECT user, int vis)
{
if (vis) {
  if (user->vis) {
    write_user(user,"You are already visible.\n");  return;
    }
  write_user(user, appear_user_prompt);
  vwrite_room_except(user->room, user,appear_prompt, user->bw_recap);
  user->vis=1;
  return;
  }
if (!user->vis) {
  write_user(user,"You are already invisible.\n");  return;
  }
write_user(user, disapear_user_prompt);
vwrite_room_except(user->room,user, disapear_prompt, user->bw_recap);
user->vis=0;
}


/*** Set the character mode echo on or off. This is only for users logging in
     via a character mode client, those using a line mode client (eg unix
     telnet) will see no effect. ***/
void toggle_charecho(UR_OBJECT user)
{
	if (!user->charmode_echo) {
		write_user(user,"Echoing for character mode clients ~FGON~RS.\n");
		user->charmode_echo=1;
		}
	else {
		write_user(user,"Echoing for character mode clients ~FROFF~RS.\n");
		user->charmode_echo=0;
		}
	if (user->room==NULL) prompt(user);
}



/*** A newbie is requesting an account. Get his email address off him so we
     can validate who he is before we promote him and let him loose as a 
     proper user. ***/
void account_request(UR_OBJECT user, char *inpstr)
{
if (user->level>NEW) {
  write_user(user,"Tento prikaz je len pre novych juzerov.\n");
  return;
  }
/* stop them from requesting an account twice */
if (BIT_TEST(user->accreq,3)) {
  write_user(user,"Uz mas podany request.\n");
  return;
  }
if (word_count<2) {
  write_usage(user,"accreq <email + nejake info>");
  return;
  }
if (!validate_email(inpstr)) {
  write_user(user,"That email address format is incorrect.  Correct format: user@network.net\n");
  return;
  }
write_syslog(REQLOG,1,"%-*s : %s\n",USER_NAME_LEN,user->name,inpstr);
sprintf(text,"~OLSYSTEM:~RS %s poziadal%s o konto.\n",
	user->name, gnd_grm(4, user->gender));
write_level(WIZ,1,text,NULL);
write_user(user,"Account request logged.\n");
add_history(user->name,1,"Made a request for an account.\n");
if (strlen(word[1])>80) write_user(user, "Pre nastavenie adresy pre autoforwrd pouzi ~FG.set email\n");
else (strcpy(user->email, word[1]));
set_forward_email(user);
}


/*** Do AFK ***/
void afk(UR_OBJECT user, char *inpstr)
{
if (word_count>1) {
  if (!strcmp(word[1],"lock")) {
#ifdef NETLINKS
    if (user->type==REMOTE_TYPE) {
      /* This is because they might not have a local account and hence
	 they have no password to use. */
      write_user(user,"Sorry, due to software limitations remote users cannot use the lock option.\n");
      return;
      }
#endif
    inpstr=remove_first(inpstr);
    if (strlen(inpstr)>AFK_MESG_LEN) {
      write_user(user,"AFK message too long.\n");  return;
      }
    write_user(user,"You are now AFK with the session locked, enter your password to unlock it.\n");
    if (inpstr[0]) {
      strcpy(user->afk_mesg,inpstr);
      write_user(user,"AFK message set.\n");
      }
    user->afk=2;
    user->status='A';
    echo_off(user);
    }
  else {
    if (strlen(inpstr)>AFK_MESG_LEN) {
      write_user(user,"AFK message too long.\n");  return;
      }
    write_user(user,"You are now AFK, press <return> to reset.\n");
    if (inpstr[0]) {
      strcpy(user->afk_mesg,inpstr);
      write_user(user,"AFK message set.\n");
      }
    user->afk=1;
    user->status='A';
    }
  }
else {
  write_user(user,"Odteraz si STAND BY, stlac <return> na reset.\n");
  user->afk=1;
  user->status='A';
  }
if (user->vis) {
  if (user->afk_mesg[0]) vwrite_room_except(user->room,user,"%s~RS goes AFK: %s\n",user->recap,user->afk_mesg);
  else vwrite_room_except(user->room,user,"%s~RS goes AFK...\n",user->recap);
  }
clear_afk(user);
}


/**** Allow a user to delete their own account ***/
void suicide(UR_OBJECT user)
{
	if (word_count<2) {
		write_usage(user,"suicide <heslo>");
		return;
		}
	if (user->restrict[RESTRICT_SUIC]==restrict_string[RESTRICT_SUIC]) {
		write_user(user,">>>You cannot use this feature...\n");
		return;
		}
	if (strcmp((char *)crypt(word[1],crypt_salt),user->pass)) {
		write_user(user,"Password incorrect.\n");
		return;
		}
	audioprompt(user, 4, 0);
	write_user(user,"\n\07~FR~OL~LI*** WARNING - This will delete your account! ***\n\nAre you sure about this (y/n)? ");
	user->misc_op=6;  
	no_prompt=1;
}


/* Set the user attributes */
void set_attributes(UR_OBJECT user, char *inpstr)
{
	int i=0,tmp=0,setattrval=-1;
	char name[USER_NAME_LEN+1],*recname;

	if (word_count<2) {
		write_user(user, "~OL~FGWould you like to use a menu ? ~FW<~FRyes~FW/~FRno~FW>~FB :~FR");
		user->misc_op=102;
		no_prompt=1;
		return;
		}
	i=0;
	strtolower(word[1]);
	while(set_tab[i].type[0]!='*') {
		if (!strcmp(set_tab[i].type,word[1])) {
			setattrval=i;
			break;
			}
		++i;
		}
	if (setattrval==-1) {
		i=0;
		write_user(user,"Attributes you can set are:\n");
		while (set_tab[i].type[0]!='*') {
			text[0]='\0';
			vwrite_user(user,"~FT%s~RS : %s\n",set_tab[i].type,set_tab[i].desc);
			i++;
			}
		return;
		}
	write_user(user,"\n");
	switch (setattrval) {
		case SET_SHOW: show_attributes(user); return;
		case SET_GEND:
			if (word_count<3) {
				write_usage(user,"set %s m/z", set_tab[SET_GEND].type);
				return;
				}
			if (user->gender && user->level<ARCH) {
				write_user(user, "Ved uz mas nastaveny gender !\n");
				return;
				}
			inpstr=remove_first(inpstr);
			inpstr=colour_com_strip(inpstr);
			inpstr[0]=tolower(inpstr[0]);
			tmp=user->gender;
			switch (inpstr[0]) {
				case 'm' :
					user->gender=MALE;
					break;
				case 'z' :
					user->gender=FEMALE;
					break;
				case 'n' :
				case 'd' :
					if (user->level >= GOD) user->gender=NEUTER;
					break;
				default  :
					write_user(user, "Take pohlavie nepoznam, ty hej ?\n");
					return;
				} /* end switch */
			vwrite_user(user,"Pohlavie nastavene na ~OL%s~RS\n", sex[user->gender]);
			syspp->acounter[tmp]--;
			syspp->acounter[user->gender]++;
			if (syspp->acounter[user->gender]>syspp->mcounter[user->gender]) {
				syspp->mcounter[user->gender]++;
				save_counters();
				}
			if (amsys->auto_promote && user->gender) check_autopromote(user,1);
			if (user->gender!=tmp) {
				nick_grm(user);
				write_user(user, "Tvoje sklonovanie mena bolo nastavene nasledovne:\n");
				show_nick_grm(user, user);
				write_user(user, "Ak mas proti tomu nejake vyhrady, kontaktuj adminov\n\n");
				}
			return;
		case SET_AGE:
			if (word_count<3) {
				write_usage(user,"set %s <age>", set_tab[SET_AGE].type);
				return;
				}
			tmp=atoi(word[2]);
			if (tmp<1 || tmp>120) {
				write_user(user,"You can only set your age range between 1 and 120.\n");
				return;
				}
			user->age=tmp;
			vwrite_user(user,"Age now set to: %d\n",user->age);
			return;
		case SET_EMAIL:
			inpstr=remove_first(inpstr);
			inpstr=colour_com_strip(inpstr);
			if (!inpstr[0]) strcpy(user->email,"#UNSET");
			else if (strlen(inpstr)>80) {
				write_user(user,"The maximum email length you can have is 80 characters.\n");
				return;
				}
			else {
				if (!validate_email(inpstr)) {
					write_user(user,"That email address format is incorrect.  Correct format: user@network.net\n");
					return;
					}
				strcpy(user->email,inpstr);
				} /* end else */
			if (!strcmp(user->email,"#UNSET")) write_user(user,"Email set to : ~FRunset\n");
			else vwrite_user(user,"Email set to : ~FT%s\n",user->email);
			set_forward_email(user);
			return;
		case SET_HOMEP:
			inpstr=remove_first(inpstr);
			inpstr=colour_com_strip(inpstr);
			if (!inpstr[0]) strcpy(user->homepage,"#UNSET");
			else if (strlen(inpstr)>80) {
				write_user(user,"The maximum homepage length you can have is 80 characters.\n");
				return;
				}
			else strcpy(user->homepage,inpstr);
			if (!strcmp(user->homepage,"#UNSET")) write_user(user,"Homepage set to : ~FRunset\n");
			else vwrite_user(user,"Homepage set to : ~FT%s\n",user->homepage);
			return;
		case SET_HIDEEMAIL:
			user->hideemail=!user->hideemail;
			vwrite_user(user,"Email showing now %s.\n", offon[user->hideemail]);
			return;
		case SET_WRAP:
			user->wrap=!user->wrap;
			vwrite_user(user,"Word wrap now %s\n", offon[user->wrap]);
			return;
		case SET_PAGER:
			if (word_count<3) {
				write_usage(user,"set %s <length>", set_tab[SET_PAGER].type);
				return;
				}
			user->pager=atoi(word[2]);
			if (user->pager<MAX_LINES || user->pager>99) {
				vwrite_user(user,"Pager can only be set between %d and 99 - setting to default\n",MAX_LINES);
				user->pager=23;
				}
			vwrite_user(user,"Pager length now set to: %d\n",user->pager);
			return;
		case SET_COLOUR:
			if (word_count<3) {
				write_usage(user,"set %s <mod>|[test]", set_tab[SET_COLOUR].type);
				vwrite_user(user, "Najvyssi mozny mod: %d\n",NUM_COLMODS);
				vwrite_user(user, "Aktualny mod: %d\n",user->colour);
				return;
				}
			if (!strcmp(word[2], "test")) {
				display_colour(user);
				return;
				}
			if (!is_number(word[2])) {
				write_user(user,"nespravny parameter\n");
				return;
				}
			tmp=atoi(word[2]);
			if (tmp>NUM_COLMODS || tmp<0) {
				vwrite_user(user, "Mozny rozsah modov: 0 - %d\n",NUM_COLMODS);
				return;
				}
			user->colour=tmp;
			vwrite_user(user, "Nastaveny mod farieb: %d\n",user->colour);
			if (user->room==NULL) prompt(user);
			return;
		case SET_ROOM:
			switch (user->lroom) {
				case 0:
					user->lroom=1;
					write_user(user,"You will log on into the room you left from.\n");
					break;
				case 1:
					user->lroom=0;
					write_user(user,"You will log on into the main room.\n");
					break;
				}
			return;
		case SET_FWD:
			if (!user->email[0] || !strcmp(user->email,"#UNSET")) {
				write_user(user,"You have not yet set your email address - autofwd cannot be used until you do.\n");
				return;
				}
			if (!user->mail_verified) {
				write_user(user,"You have not yet verified your email - autofwd cannot be used until you do.\n");
				return;
				}
			switch (user->autofwd) {
				case 0:
					user->autofwd=1;
					write_user(user,"You will also receive smails via email.\n");
					break;
				case 1:
					user->autofwd=0;
					write_user(user,"You will no longer receive smails via email.\n");
					break;
				}
			return;
		case SET_PASSWD:
			switch (user->show_pass) {
				case 0:
					user->show_pass=1;
					write_user(user,"You will now see your password when entering it at login.\n");
					break;
				case 1:
					user->show_pass=0;
					write_user(user,"You will no longer see your password when entering it at login.\n");
					break;
				}
			return;
		case SET_RDESC:
			switch (user->show_rdesc) {
				case 0:
					user->show_rdesc=1;
					write_user(user,"You will now see the room descriptions.\n");
					break;
				case 1:
					user->show_rdesc=0;
					write_user(user,"You will no longer see the room descriptions.\n");
					break;
				}
			return;
		case SET_COMMAND:
			if (word_count<3) {
				write_usage(user,"set %s <#typ>", set_tab[SET_COMMAND].type);
				return;
				}
			tmp=atoi(word[2]);
			if (tmp<1 || tmp>NUM_HELP) {
				vwrite_user(user,"Help type can only be set between 1 & %d - setting to default\n", NUM_HELP);
				user->cmd_type=1;
				}
			else user->cmd_type=tmp;
			vwrite_user(user,"Help type now set to: %s\n", help_style[user->cmd_type]);
			return;
		case SET_RECAP:
			if (!amsys->allow_recaps) {
				write_user(user,"Sorry, names cannot be recapped at this present time.\n");
				return;
				}
			if (word_count<3) {
				write_usage(user,"set %s <meno ako ho chces mat>", set_tab[SET_RECAP].type);
				return;
				}
			if (strstr(word[2], "~FK") && user->level<ROOT) {
				write_user(user, "You can't have black in your name!\n");
				return;
				}
			if (strlen(word[2])>(USER_NAME_LEN+USER_NAME_LEN*3-1)) {
				write_user(user, "That recapped name is too long!\n");
				return;
				}
			recname=colour_com_strip(word[2]);
			strcpy(name,recname);
			strtolower(name);
			name[0]=toupper(name[0]);
			if (strcmp(user->name,name)) {
				write_user(user,"The recapped name still has to match your proper name.\n");
				return;
				}
			strcpy(user->recap,word[2]);
			strcpy(user->bw_recap,recname);
			vwrite_user(user,"Your name will now appear as \"%s~RS\" on the 'who', 'examine', tells, etc.\n",user->recap);
			return;
		case SET_ICQ:
			strcpy(word[2],colour_com_strip(word[2]));
			if (!word[2][0]) strcpy(user->icq,"#UNSET");
			else if (strlen(word[2])>ICQ_LEN) {
				vwrite_user(user,"The maximum ICQ UIN length you can have is %d characters.\n", ICQ_LEN);
				return;
				}
			else strcpy(user->icq,word[2]);
			if (!strcmp(user->icq,"#UNSET")) write_user(user,"ICQ number set to : ~FRunset\n");
			else vwrite_user(user,"ICQ number set to : ~FT%s\n",user->icq);
			return;
		case SET_ALERT:
			switch (user->alert) {
				case 0:
					user->alert=1;
					write_user(user,"You will now be alerted if anyone on your friends list logs on.\n");
					break;
				case 1:
					user->alert=0;
					write_user(user,"You will no longer be alerted if anyone on your friends list logs on.\n");
					break;
				}
			return;
		case SET_AUDIO:
			user->pueblo_mm=!user->pueblo_mm;
			vwrite_user(user, "'Pueblo Audio Prompt' now %s\n", offon[user->pueblo_mm]);
			return;
		case SET_PPA:
			user->pueblo_pg=!user->pueblo_pg;
			vwrite_user(user, "'Pueblo Pager Audio' now %s\n", offon[user->pueblo_pg]);
			return;
		case SET_VOICE:
			user->voiceprompt=!user->voiceprompt;
			vwrite_user(user, "'audio prompt voice gender' nastavene na %s\n",
				sex[(!(user->voiceprompt-1))+1]);
			if (!user->pueblo) write_user(user,"This function only works when connected using the Pueblo telnet client.\n");
			return;
		case SET_XTERM:
			user->xterm=!user->xterm;
			vwrite_user(user, "xterm kompatibilita %s\n", offon[user->xterm]);
			return;
		case SET_MODE:
#ifdef NETLINKS
			if (user->room==NULL) {
				vwrite_user(user, "Sorrac, momentalne ako vzdialen%s user%s nemozes pouzit tento prikaz\n", gnd_grm(1, user->gender), gnd_grm(2, user->gender));
				return;
				}
#endif
			user->command_mode=!user->command_mode;
			vwrite_user(user, "odteraz si v %s mode\n", user->command_mode?"kecacom":"prikazovom");
			return;
		case SET_PROMPT:
#ifdef NETLINKS
			if (user->room==NULL) {
				vwrite_user(user, "Sorrac, momentalne ako vzdialen%s user%s nemozes pouzit tento prikaz\n", gnd_grm(1, user->gender), gnd_grm(2, user->gender));
				return;
				}
#endif
			if (word_count<3) {
				if (user->prompt>0) vwrite_user(user, "Aktualny prompt: %s\n", prompt_tab[user->prompt].name);
				else write_user(user, "Aktualny prompt: vlastny\n");
				write_usage(user, "set %s <typ>|<str>", set_tab[SET_PROMPT].type);
				return;
				}
			if (is_number(word[2])) {
				tmp=atoi(word[2]);
				if (tmp>(NUM_PROMPT-1)) {
					vwrite_user(user, "Prompt moze byt 0 - %d alebo vlastny retazec\n", NUM_PROMPT);
					return;
					}
				user->prompt=tmp;
				strcpy(user->prompt_str, prompt_tab[user->prompt].str);
				vwrite_user(user, "Prompt nastaveny na: %s\n", prompt_tab[user->prompt].name);
				return;
				}
			if (strlen(word[2])>PROMPT_LEN) {
				vwrite_user(user, "Pridlhy prompt, max. dlzka: %d\n", PROMPT_LEN);
				return;
				}
			user->prompt=-1;
			sprintf(user->prompt_str, "%.*s", PROMPT_LEN, word[2]);
			user->prompt_str[PROMPT_LEN]='\0';
			write_user(user, "Prompt nastaveny\n");
			return;
		case SET_WHO:
			if (word_count<3) {
				write_usage(user,"set %s <#typ>", set_tab[SET_WHO].type);
				return;
				}
			tmp=atoi(word[2]);
			if (tmp<1 || tmp>NUM_WHO) {
				vwrite_user(user,"Who type can only be set between 1 & %d - setting to default\n", NUM_WHO);
				user->who_type=1;
				}
			else user->who_type=tmp;
			vwrite_user(user,"Who type now set to: %s\n", who_style[user->who_type]);
			return;
		} /* end main switch */
}


/* The set menu... very very very handy... for newbies */
int set_ops(UR_OBJECT user, char *inpstr)
{
	char *colour_com_strip();
	char temp[ARR_SIZE],gender[15],mode[20],*name;
	int which,val,hlp;

	if (!user->vis) name=invisname;
	else name=user->recap;
	temp[0]='\0';
	val=0;
	if (inpstr[0]=='0') {
		hlp=-1;
		val=1;
		}
	else for (hlp=0; set_tab[hlp].cmenu!='*'; hlp++)
		if (toupper(inpstr[0])==set_tab[hlp].cmenu) {
			val=1;
			break;
		}
	switch (user->set_op) {
		case 1 :
			switch (hlp) {
				case SET_GEND:
					if (user->gender && user->level<ARCH) {
						write_user(user, "Ved uz mas nastaveny gender !\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					vwrite_user(user, "Aktualny gender: %s\n", sex[user->gender]);
					write_user(user,"~CTEnter new gender~CB: ");
					user->set_op=2;
					return;
				case SET_AGE:
					if (user->age) {
						vwrite_user(user,"~FGYour age currently is~CB: [~RS%d~CB]\n", user->age);
						write_user(user,"~CTEnter new age~CB: ");
						user->set_op=3;
						return;
						}
					write_user(user,"~CTHow old are you?~CB: ");
					user->set_op=3;
					return;
				case SET_EMAIL:
					if (!strcmp(user->email, "#UNSET")) write_user(user,"~FGEmail currently set at~CB: [~RSunset~CB]\n");
					else vwrite_user(user,"~FGEmail currently set at~CB: [~RS%s~CB]\n", user->email);
					write_user(user,"~CTEnter new email address~CB: ");
					user->set_op=4;
					return;
				case SET_HOMEP:
					if (strcmp(user->homepage, "#UNSET")) {
						vwrite_user(user,"~FGWebpage currently set at~CB: [~RS%s~CB]\n",user->homepage);
						write_user(user,"~CTEnter a new homepage address~CB =>~RS http://");
						user->set_op=5;
						return;
						}
					write_user(user,"~CTEnter a homepage address~CB:~RS http://");
					user->set_op=5;
					return;
				case SET_HIDEEMAIL:
					if (user->hideemail) {
						write_user(user,"~FGYou are no longer hiding your email address...\n");
						user->hideemail=0;
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return; 
						}
					write_user(user,"~FGYou hide your email from other users...\n");
					user->hideemail=1;
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_WRAP:
					user->wrap=!user->wrap;
					vwrite_user(user,"~FGWord wrap now ~OL%s\n", offon[user->wrap]);
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_PAGER:
					vwrite_user(user, "~FGYour pager currently is ~CB: [~RS%d~CB]\n", user->pager);
					write_user(user, "~CTEnter new pager value~CB: ");
					user->set_op=6;
					return;
				case SET_COLOUR:
					vwrite_user(user, "~FGYour colour mode is ~CB: [~RS%d~CB]\n", user->colour);
					vwrite_user(user, "~CTEnter colour mode ~FW1~%d ~CB: ", NUM_COLMODS);
					user->set_op=7;
					return;
				case SET_ROOM:
					user->lroom=!user->lroom;
					vwrite_user(user, "You will log on into the %s\n",
						user->lroom?"room you left from":"main room");
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_FWD:
					if (!user->email[0] || !strcmp(user->email, "#UNSET")) {
						write_user(user, "You have no yet set your email address - autofwd cannot be used until you do.\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					if (!user->mail_verified) {
						write_user(user, "You have no yet verified your email - autofwd cannot be used until you do.\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					user->autofwd=!user->autofwd;
					vwrite_user(user, "Autofwd now %s\n", offon[user->autofwd]);
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_PASSWD:
					if (user->show_pass) {
						write_user(user, "You will now see your password when entering it at login\n");
						write_user(user,center(continue1,81));
						user->show_pass=0;
						user->set_op=-1;
						return;
						}
					write_user(user, "You will no longer see your password when entering it at login\n");
					write_user(user,center(continue1,81));
					user->show_pass=1;
					user->set_op=-1;
					return;
				case SET_RDESC:
					if (user->show_rdesc) {
						write_user(user, "You will now see the room descriptions\n");
						write_user(user,center(continue1,81));
						user->show_rdesc=0;
						user->set_op=-1;
						return;
						}
					write_user(user, "You will no longer see the room descriptions\n");
					write_user(user,center(continue1,81));
					user->show_rdesc=1;
					user->set_op=-1;
					return;
				case SET_COMMAND:
					vwrite_user(user,"~FGYour help type is currently~CB: [~RS%d - %s~CB]\n", user->cmd_type, help_style[user->cmd_type]);
					write_user(user,"~FMPossible help types are~CB:\n");
					for (which=1; which<=NUM_HELP; ++which) {
						vwrite_user(user,"~CG%d~CB = ~CT%s\n", which, help_style[which]);
						}
					write_user(user,"~CTYour choice~CB: ");
					user->set_op=8;
					return;
				case SET_RECAP:
					if (!amsys->allow_recaps) {
						write_user(user, "Sorry, names cannot be recapped at this present time");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					vwrite_user(user,"~FGName capitalization currently set at~CB: [~RS%s~RS~CB]\n", user->recap);
					write_user(user,"~CTEnter new capitalization~CB: ");
					user->set_op=9;
					return;
				case SET_ICQ:
					if (user->icq) {
						if (strcmp(user->icq, "#UNSET")) vwrite_user(user,"~FGYour ICQ number is set at~CB: [~RS%s~CB]\n", user->icq);
						else write_user(user,"~FGYour ICQ number is set at~CB: [~RSunset~CB]\n");
						write_user(user,"~CTEnter new ICQ number~CB: ");
						user->set_op=10;
						return;
						}
					write_user(user,"~CTEnter an ICQ number~CB: ");
					user->set_op=10;
					return;
				case SET_ALERT:
					if (user->alert) {
						write_user(user, "You will now be alerted if anyone on your notify list logs on\n");
						user->alert=0;
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					write_user(user, "You will no longer be alerted if anyone on your notify list logs on\n");
					user->alert=1;
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_AUDIO:
					user->pueblo_mm=!user->pueblo_mm;
					vwrite_user(user, "'Pueblo Audio Prompt' now %s\n", offon[user->pueblo_mm]);
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_PPA:
					user->pueblo_pg=!user->pueblo_pg;
					vwrite_user(user, "'Pueblo Pager Audio' now %s\n", offon[user->pueblo_pg]);
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_VOICE:
					user->voiceprompt=!user->voiceprompt;
					vwrite_user(user, "Audio Prompt Voice Gender' nastavene na %s\n",
						sex[(!(user->voiceprompt-1))+1]);
					if (!user->pueblo) write_user(user, "This function only works when connected using the Pueblo telnet client.\n");
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_XTERM:
					user->xterm=!user->xterm;
					vwrite_user(user, "Xterm kompatibilita %s\n", offon[user->xterm]);
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_MODE:
					if (user->room==NULL) {
						write_user(user, "teraz nemozes zmenit tento parameter\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return; 
						}
					if (user->command_mode) {
						write_user(user,"~FGNow in ~CRSPEECH~FG mode!\n");
						user->command_mode=0;
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return; 
						}
					write_user(user,"~FGNow in ~CRCOMMAND~FG mode!\n");
					user->command_mode=1;
					write_user(user,center(continue1,81));
					user->set_op=-1;
					return;
				case SET_PROMPT:
					vwrite_user(user, "~FGAktualny prompt~CB: [~RS%d ~CW-~RS %s~CB].\n", user->prompt, prompt_tab[user->prompt].name);
					vwrite_user(user, "~CTZadaj novy prompt~CB [~FR0~FW-~FR%d~FW|~FR<str>~CB]: ~RS", NUM_PROMPT-1);
					user->set_op=11;
					return;
				case SET_WHO:
					vwrite_user(user,"~FGYour who type is currently~CB: [~RS%d - %s~CB]\n", user->who_type, who_style[user->who_type]);
					write_user(user,"~CTYour choice~CB: ");
					user->set_op=12;
					return;
				case -1:
					vwrite_room_except(user->room, user,"~FT%s ~CGleaves the setup menu!\n",name);
					user->ignall=user->ignall_store;
					user->set_mode=0;
					user->set_op=0;
					user->status='a';
					prompt(user);
					return;
				default  :
					write_user(user,"~CRYou have selected an invalid choice....\n");
					write_user(user,center(continue1,81));
					user->set_op=-1;
				} /* end of switch (hlp) */
			return 1;

		case 2: /* SET_GEND */
			hlp=user->gender;
			inpstr=colour_com_strip(inpstr);
			switch (toupper(inpstr[0])) {
				case 'M':
					if (user->gender & FEMALE) {
						write_user(user,"~FGGender switched to ~CRMALE\n");
						user->gender=MALE;
						syspp->acounter[hlp]--;
						syspp->acounter[user->gender]++;
						user->set_op=-1;
						goto CH_GEND;
						}
					if (user->gender & MALE) {
						write_user(user,"~FGYes, we know that your male...\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					user->gender=MALE;
					syspp->acounter[hlp]--;
					syspp->acounter[user->gender]++;
					write_user(user,"~FGGender set to ~CRMALE!\n");
					user->set_op=-1;
					goto CH_GEND;
				case 'Z':
					if (user->gender & MALE) {
						write_user(user,"~FGGender switched to ~CRFEMALE\n");
						user->gender=FEMALE;
						syspp->acounter[hlp]--;
						syspp->acounter[user->gender]++;
						user->set_op=-1;
						goto CH_GEND;
						}
					if (user->gender & FEMALE) {
						write_user(user,"~FGYes, we know that your female...\n");
						write_user(user,center(continue1,81));
						user->set_op=-1;
						return;
						}
					user->gender=FEMALE;
					syspp->acounter[hlp]--;
					syspp->acounter[user->gender]++;
					write_user(user,"~FGGender set to ~CRFEMALE!\n");
					user->set_op=-1;
					goto CH_GEND;
				case 'N':
				case 'D':
					if (user->level>=GOD) {
						if (user->gender & NEUTER) {
							write_user(user,"~FGYes, we know that your female...\n");
							write_user(user,center(continue1,81));
							user->set_op=-1;
							return;
							}
						user->gender=NEUTER;
						syspp->acounter[hlp]--;
						syspp->acounter[user->gender]++;
						write_user(user,"~FGGender set to ~CRNEUTER!\n");
						user->set_op=-1;
						goto CH_GEND;
						}
				default :
					write_user(user,"~FGInvalid gender, please choose either male or female!\n");
					write_user(user,"~CB-=> ");
					user->set_op=2;
					return;
				}
CH_GEND:
			if (syspp->acounter[user->gender]>syspp->mcounter[user->gender]) {
				syspp->mcounter[user->gender]++;
				save_counters();
				}
			if (amsys->auto_promote && user->gender) check_autopromote(user, 1);
			if (user->gender!=hlp) {
				nick_grm(user);
				write_user(user, "Tvoje sklonovanie nicku bolo nastavene nasledovne:\n");
				show_nick_grm(user, user);
				write_user(user, "Ak mas proti tomu nejake vyhrady, kontaktuj adminov\n");
				}
			write_user(user,center(continue1,81));
			return 1;

		case 3: /* SET_AGE */
			val=atoi(word[0]);
			if (!val) {
				write_user(user,"~FGInvalid age...\n");
				write_user(user,"~CTRe-enter your age~CB: ");
				user->set_op=3;
				return;
				}
			if (val<1) {
				write_user(user,"~FGI don't think so...\n");
				write_user(user,"~CTRe-enter your age~CB: ");
				user->set_op=3;
				return;
				}
			if (val>120) {
				write_user(user,"~FGThats a tad bit to old to believe!\n");
				write_user(user,"~CTRe-enter your age~CB: ");
				user->set_op=3;
				return;
				}
			user->age=val;
			vwrite_user(user,"~FGAge is now set to~CB: [~RS%d~CB]\n", user->age);
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 4: /* SET_EMAIL */
			if (!strlen(inpstr)) {
				write_user(user,"~FGWould help if you entered something ;)\n");
				write_user(user,"~CTRe-enter email address~CB: ");
				user->set_op=4;
				return;
				}
			inpstr=colour_com_strip(inpstr);
			if (!inpstr[0]) strcpy(user->email, "#UNSET");
			else if (strlen(inpstr)>80) {
				write_user(user, "The maximum email length you can have is 80 characters.\n");
				write_user(user,"~CTRe-enter email address~CB: ");
				user->set_op=4;
				return;
				}
			else {
				if (!validate_email(inpstr)) {
					write_user(user,"~FGYou have entered an invalid email address!\n");
					write_user(user,"~CTRe-enter email address~CB: ");
					user->set_op=4;
					return;
					}
				strcpy(user->email,inpstr);
				}
			if (!strcmp(user->email, "#UNSET")) write_user(user,"~FGEmail address now set to~CB: [~RSunset~CB]\n");
			else vwrite_user(user,"~FGEmail address now set to~CB: [~RS%s~CB]\n", user->email);
			set_forward_email(user);
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 5: /* SET_HOMEP */
			inpstr=colour_com_strip(inpstr);
			if (!inpstr[0]) strcpy(user->homepage, "#UNSET");
			else if (strlen(inpstr)>73) {
				write_user(user, "The maximum homepage length you can have is 73 characters\n");
				write_user(user,"~CTRe-enter a new homepage address~CB =>~RS http://");
				user->set_op=5;
				return;
				}
			else {
				sprintf(text,"http://%s",inpstr);
				strcpy(user->homepage,text);
				}
			if (strcmp(user->homepage, "#UNSET")) vwrite_user(user,"~FGHomepage now set to~CB: [~RS%s~CB]\n",user->homepage);
			else write_user(user,"~FGHomepage now set to~CB: [~RSunset~CB]\n");
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 6: /* SET_PAGER */
			user->pager=atoi(word[0]);
			if (user->pager<MAX_LINES || user->pager>99) {
				vwrite_user(user, "Pager can only be set between %d and 99 - setting to default\n");
				user->pager=23;
				}
			vwrite_user(user, "~FGPager length now set to: ~RS%d\n", user->pager);
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 7: /* SET_COLOUR */
			if (!strlen(inpstr)) {
				write_user(user,"~FGWould help if you entered something ;)\n");
				vwrite_user(user,"~FTRe-enter colour choice ~CW<~CR1~CW - ~CR%d~CW/~CRtest~CW> ~CB-=> ", NUM_COLMODS);
				user->set_op=7;
				return;
				}
			if (!strcasecmp(inpstr, "test")) {
				display_colour(user);
				vwrite_user(user,"~FTEnter colour choice ~CW<~CR1~CW - ~CR%d~CW/~CRtest~CW> ~CB-=> ", NUM_COLMODS);
				user->set_op=7;
				return;
				}
			hlp=atoi(word[0]);
			if (hlp<0 || hlp>NUM_COLMODS) {
				vwrite_user(user,"~FTInavlid choice ~CW<~CR1~CW - ~CR%d~CW/~CRtest~CW> ~CB-=> ", NUM_COLMODS);
				user->set_op=7;
				return;
				}
			user->colour=hlp;
			vwrite_user(user, "~FGColour mode now set to: ~RS%d\n", user->colour);
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 8: /* SET_COMMAND */
			val=atoi(word[0]);
			if (!val) {
				write_user(user,"~FGInvalid help style...\n");
				vwrite_user(user,"~CTRe-enter help choice 1-%d~CB: ",NUM_HELP);
				user->set_op=8;
				val=0;
				return;
				}
			if (val<1 || val>NUM_HELP) {
				write_user(user,"~FGInvalid help style...\n");
				vwrite_user(user,"~CTRe-enter help choice 1-%d~CB: ",NUM_HELP);
				user->set_op=8;
				val=0;
				return;
				}
			user->cmd_type=val;
			vwrite_user(user,"~FGHelp style now set to~CB: [~CY%s~CB]\n", help_style[user->cmd_type]);
			write_user(user,center(continue1,81));
			user->set_op=-1;
			return 1;

		case 9: /* SET_RECAP */
			if (!strlen(inpstr)) {
				write_user(user,"~FGWould help if you entered something ;)\n");
				write_user(user,"~CTRe-enter recapped name~CB: ");
				user->set_op=9;
				return;
				}
			if (strstr(inpstr,"~FK") && user->level<ROOT) {
				write_user(user,"~FGYou can't have black in your name!\n");
				write_user(user,"~CTRe-enter recapped name~CB: ");
				user->set_op=9;
				return;
				}
			if (strlen(inpstr)>(USER_NAME_LEN+USER_NAME_LEN*3-1)) {
				write_user(user,"~FGThat recapped name is too long!\n");
				write_user(user,"~CTRe-enter recapped name~CB: ");
				user->set_op=9;
				return;
				}
			strcpy(temp,colour_com_strip(inpstr));
			if (strcasecmp(temp,user->name) && user->level<ROOT) {
				write_user(user,"~FGThats not your name...\n");
				write_user(user,"~CTRe-enter recapped name~CB: ");
				user->set_op=9;
				return;
				}
			strcpy(user->recap, inpstr);
			strcpy(user->bw_recap, temp);
			vwrite_user(user,"~FGYou will now be known as~CB: [~RS%s~RS~CB]\n", user->recap);
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 10: /* SET_ICQ */
			inpstr=colour_com_strip(inpstr);
			if (!inpstr[0]) strcpy(user->icq, "#UNSET");
			else if (strlen(inpstr)>ICQ_LEN) {
				vwrite_user(user, "The Maximnum ICQ UIN length you can have is %d characters.\n", ICQ_LEN);
				write_user(user,"~CTRe-enter an ICQ number~CB: ");
				user->set_op=10;
				return;
				}
			else strcpy(user->icq, word[0]);
			if (strcmp(user->icq, "#UNSET")) vwrite_user(user,"~FGICQ number now set to~CB: [~RS%s~CB]\n",user->icq);
			else write_user(user,"~FGICQ number now set to~CB: [~RSunset~CB]\n");
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 11: /* SET_PROMPT */
			if (is_number(inpstr)) {
				val=atoi(inpstr);
				if (val>(NUM_PROMPT-1)) {
					vwrite_user(user, "Maximalne cislo promptu je %d.\n", NUM_PROMPT-1);
					write_user(user,"~CTRe-enter prompt~CB: ");
					user->set_op=11;
					return;
					}
				user->prompt=val;
				vwrite_user(user, "Prompt nastaveny na typ %s\n", prompt_tab[user->prompt].name);
				strcpy(user->prompt_str, prompt_tab[user->prompt].str);
				write_user(user,center(continue1,81));
				user->set_op=-1;
				return 1;
				}
			if (strlen(inpstr)>PROMPT_LEN) {
				vwrite_user(user, "Maximalna dlzka promptu je %d.\n", PROMPT_LEN);
				write_user(user,"~CTRe-enter prompt~CB: ");
				user->set_op=11;
				return;
				}
			user->prompt=-1;
			strcpy(user->prompt_str, inpstr);
			write_user(user, "Prompt nastaveny\n");
			write_user(user,center(continue1,81));
		        user->set_op=-1;
			return 1;

		case 12: /* SET_WHO */
			if (!strcmp(word[0], "list")) {
				write_user(user,"~FMPossible who types are~CB:\n");
				for (which=1; which<=NUM_WHO; ++which) {
					vwrite_user(user,"~CG%d~CB = ~CT%s\n", which, who_style[which]);
					}
				write_user(user,"~CTYour choice~CB: ");
				user->set_op=12;
				return;
				}
			val=atoi(word[0]);
			if (!val) {
				write_user(user,"~FGInvalid who style...\n");
				vwrite_user(user,"~CTRe-enter who choice 1-%d|list~CB: ",NUM_WHO);
				user->set_op=12;
				return;
				}
			if (val<1 || val>NUM_WHO) {
				write_user(user,"~FGInvalid who style...\n");
				vwrite_user(user,"~CTRe-enter who choice 1-%d|list~CB: ",NUM_WHO);
				user->set_op=12;
				return;
				}
			user->who_type=val;
			vwrite_user(user,"~FGWho style now set to~CB: [~CY%s~CB]\n", who_style[user->who_type]);
			write_user(user,center(continue1,81));
			user->set_op=-1;
			return 1;

		case -1:
			temp[0]='\0';
			which=0;
			val=0;
			print_menu(user, MENU_SET);
			user->set_op=1;
			return 1;
		}
	return 0;
}


/** Force a user to become visible **/
void make_vis(UR_OBJECT user)
{
UR_OBJECT user2;

if (word_count<2) {
  write_usage(user,"makevis <user>");
  return;
  }
if (!(user2=get_user_name(user,word[1]))) {
  write_user(user,notloggedon);  return;
  }
if (user==user2) {
  write_user(user,"There is an easier way to make yourself visible!\n");
  return;
  }
if (user2->vis) {
  vwrite_user(user,"%s~RS is already visible.\n",user2->recap);
  return;
  }
if (user2->level>user->level) {
  vwrite_user(user,"%s~RS cannot be forced visible.\n",user2->recap);
  return;
  }
user2->vis=1;
vwrite_user(user,"You force %s~RS to become visible.\n",user2->recap);
write_user(user2,"You have been forced to become visible.\n");
vwrite_room_except(user2->room,user2,"You see %s~RS mysteriously emerge from the shadows!\n",user2->recap);
}


/** Force a user to become invisible **/
void make_invis(UR_OBJECT user)
{
UR_OBJECT user2;

if (word_count<2) {
  write_usage(user,"makeinvis <user>");
  return;
  }
if (!(user2=get_user_name(user,word[1]))) {
  write_user(user,notloggedon);  return;
  }
if (user==user2) {
  write_user(user,"There is an easier way to make yourself invisible!\n");
  return;
  }
if (!user2->vis) {
  vwrite_user(user,"%s~RS is already invisible.\n",user2->recap);
  return;
  }
if (user2->level>user->level) {
  vwrite_user(user,"%s~RS cannot be forced invisible.\n",user2->recap);
  return;
  }
user2->vis=0;
vwrite_user(user,"You force %s~RS to become invisible.\n",user2->recap);
write_user(user2,"You have been forced to become invisible.\n");
vwrite_room_except(user2->room,user2,"You see %s~RS mysteriously disappear into the shadows!\n",user2->recap);
}


/* Shows when a user was last logged on */
void show_last_login(UR_OBJECT user)
{
UR_OBJECT u;
int timelen,days,hours,mins,i;
char line[ARR_SIZE],tmp[ARR_SIZE];

if (word_count>2) {
  write_usage(user,"last [<user>]");
  return;
  }
/* if checking last on a user */
if (word_count==2) {
  word[1][0]=toupper(word[1][0]);
  if (!strcmp(word[1],user->name)) {
    write_user(user,"You are already logged on!\n");
    return;
    }
  if ((u=get_user(word[1]))) {
    vwrite_user(user,"%s is currently logged on.\n",u->name);
    return;
    }
  /* User not logged in */
  if ((u=create_user())==NULL) {
    vwrite_user(user,"%s: unable to create temporary user object.\n",syserror);
    write_syslog(SYSLOG,0,"ERROR: Unable to create temporary user object in show_last().\n");
    return;
    }
  strcpy(u->name,word[1]);
  if (!load_user_details(u)) {
    write_user(user,nosuchuser);
    destruct_user(u);
    destructed=0;
    return;
    }
  timelen=(int)(time(0)-u->last_login);
  days=timelen/86400;
  hours=(timelen%86400)/3600;
  mins=(timelen%3600)/60;
  write_user(user,"\n+----------------------------------------------------------------------------+\n");
  vwrite_user(user,"| ~FT~OLLast login details of %-52s~RS |\n",u->name);
  write_user(user,"+----------------------------------------------------------------------------+\n");
  strcpy(tmp,ctime((time_t *)&(u->last_login)));
  tmp[strlen(tmp)-1]='\0';
  vwrite_user(user,"| Was last logged in %-55s |\n",tmp);
  sprintf(tmp,"Which was %d day%s, %d hour%s and %d minute%s ago",days,PLTEXT_S(days),hours,PLTEXT_S(hours),mins,PLTEXT_S(mins));
  vwrite_user(user,"| %-74s |\n",tmp);
  sprintf(tmp,"Was on for %d hour%s and %d minute%s",
	  u->last_login_len/3600,PLTEXT_S(u->last_login_len/3600),(u->last_login_len%3600)/60,PLTEXT_S((u->last_login_len%3600)/60));
  vwrite_user(user,"| %-74s |\n",tmp);
  write_user(user,"+----------------------------------------------------------------------------+\n\n");
  destruct_user(u);
  destructed=0;
  return;
  } /* end if */
/* if checking all of the last users to log on */
/* get each line of the logins and check if that user is still on & print out the result. */
write_user(user,"\n+----------------------------------------------------------------------------+\n");
write_user(user,"| ~FT~OLThe last users to have logged in~RS                                           |\n");
write_user(user,"+----------------------------------------------------------------------------+\n");
for (i=0;i<LASTLOGON_NUM;i++) {
  if (!last_login_info[i].name[0]) continue;
  if (last_login_info[i].on && (!(u=get_user(last_login_info[i].name))->vis && user->level<WIZ)) continue;
  sprintf(line,"%s %s",last_login_info[i].name,last_login_info[i].time);
  if (last_login_info[i].on) sprintf(text,"| %-67s ~OL~FYONLINE~RS |\n",line);
  else sprintf(text,"| %-74s |\n",line);
  write_user(user,text);
  } /* end for */
write_user(user,"+----------------------------------------------------------------------------+\n\n");
return;
}


/* Create an account for a user if new users from their site have been
   banned and they want to log on - you know they aint a trouble maker, etc */
void create_account(UR_OBJECT user)
{
UR_OBJECT u;
int i;

if (word_count<3) {
  write_usage(user,"create <meno> <heslo>");
  return;
  }
/* user logged on */
if ((u=get_user(word[1]))!=NULL) {
  write_user(user,"You cannot create with the name of an existing user!\n");
  return;
  }
/* user not logged on */
if ((u=create_user())==NULL) {
  vwrite_user(user,"%s: unable to create temporary user session.\n",syserror);
  write_syslog(SYSLOG,0,"ERROR: Unable to create temporary user session in create_account().\n");
  return;
  }
if (strlen(word[1])>USER_NAME_LEN) {
  write_user(user,"Name was too long - account not created.\n");
  destruct_user(u);  destructed=0;  return;
  }
if (strlen(word[1])<USER_MIN_LEN) {
  write_user(user,"Name was too short - account not created.\n");
  destruct_user(u);  destructed=0;  return;
  }
for (i=0;i<strlen(word[1]);++i) {
  if (!isalpha(word[1][i])) {
    write_user(user,"You can't have anything but letters in the name - account not created.\n\n");
    destruct_user(u);  destructed=0;  return;
    }
  }
if (contains_swearing(word[1])) {
  write_user(user,"You cannot use a name like that - account not created.\n\n");
  destruct_user(u);  destructed=0;  return;
  }
if (strlen(word[2])>PASS_LEN) {
  write_user(user,"Password was too long - account not created.\n");
  destruct_user(u);  destructed=0;  return;
  }
if (strlen(word[2])<PASS_MIN_LEN) {
  write_user(user,"Password was too short - account not created.\n");
  destruct_user(u);  destructed=0;  return;
  }
strcpy(u->name,word[1]);
if (!load_user_details(u)) {
  strcpy(u->pass,(char *)crypt(word[2],crypt_salt));
  strcpy(u->recap,u->name);
  strcpy(u->desc,"is a newbie needing a desc.");
  strcpy(u->in_phrase,"wanders in.");
  strcpy(u->out_phrase,"wanders out");
  strcpy(u->last_site,"created_account");
  strcpy(u->site,u->last_site);
  strcpy(u->logout_room,"<none>");
  strcpy(u->verify_code,"#NONE");
  strcpy(u->email,"#UNSET");
  strcpy(u->homepage,"#UNSET");
  strcpy(u->icq,"#UNSET");
  u->prompt=amsys->prompt_def;
  u->charmode_echo=0;
  u->room=room_first;
  u->level=NEW;
  u->unarrest=NEW;
  strcpy(u->restrict, RESTRICT_MASK);
  amsys->level_count[u->level]++;
  amsys->user_count++;
  save_user_details(u,0);
  add_user_node(u->name,u->level);
  add_user_date_node(user->name,(long_date(1)));
  sprintf(text,"Was manually created by %s.\n",user->name);
  add_history(u->name,1,text);
  vwrite_user(user,"You have created an account with the name \"~FT%s~RS\" and password \"~FG%s~RS\".\n",u->name,word[2]);
  write_syslog(SYSLOG,1,"%s created a new account with the name '%s'\n",user->name,u->name);
  destruct_user(u);
  destructed=0;
  return;
  }
write_user(user,"You cannot create an account with the name of an existing user!\n");
destruct_user(u);
destructed=0;
}



/*****************************************************************************/
/* Doplnene funkcie                                                          */
/*****************************************************************************/

void set_ualarm(UR_OBJECT user)
{
	if (word_count<2) {
		write_usage(user,"alarm stop/<cas v min>");
		return;
		}
	if (!strcmp(word[1],"stop")) {
		if (user->alarm) {
			write_user(user,"Budik je odteraz vypnuty\n");
			user->alarm=0;
			user->atime=0;
			return;
			}
		else {
			write_user(user,"Ved nemas zapnuty budik !\n");
			return;
			}
		}
	if ((is_number(word[1])) && (user->alarm)) {
		write_user(user,"Ved uz mas nastaveny budik\n");
		vwrite_user(user, "Bude ti zvonit za %d s\n", user->atime);
		return;
		}
	if (!(is_number(word[1]))) {
		write_user(user,"Nekorektny parameter\n");
		return;
		}
	user->alarm=1;
	user->atime=60*(atoi(word[1]));
	vwrite_user(user, "Budik nastaveny na %d min\n", user->atime/60);
}


void set_follow(UR_OBJECT user)
{

	if (word_count<2) {
		write_usage(user, "follow [<user>]|[-cancel]");
		if (!user->follow) write_user(user, "Momentalne nikoho nesledujes\n");
		else vwrite_user(user, "Momentalne sledujes %s\n", user->follow->name);
		return;
		}
	if (!strcmp(word[1], "-cancel")) {
		if (!user->follow) {
			write_user(user, "Ved nikoho nesledujes !\n");
			return;
			}
		vwrite_user(user, "Odteraz uz nesledujes %s\n", user->follow->name);
		user->follow=NULL;
		return;
		}
	if (user->lroom==2) {
		write_user(user, "Si prilepen%s - nemozes nikoho sledovat 1\n", gnd_grm(1, user->gender));
		return;
		}
	if (user->restrict[RESTRICT_GO]==restrict_string[RESTRICT_GO]) {
		write_user(user, ">>>You cannot have acces to another sky...\n");
		return;
		}
	if ((user->follow=get_user_name(user, word[1]))==NULL) {
		write_user(user, notloggedon);
		return;
		}
	if (user->follow==user) {
		write_user(user, "Sledovat seba ? Salies ?\n");
		return;
		}
	vwrite_user(user, "Odteraz sledujes %s\n", user->follow->name);
}

