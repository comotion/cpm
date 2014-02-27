/* #############################################################################
 * code for handling the GUI interface
 * #############################################################################
 * Copyright (C) 2005-2009 Harry Brueckner
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Contact: Harry Brueckner <harry_b@mm.st>
 *          Muenchener Strasse 12a
 *          85253 Kleinberghofen
 *          Germany
 * #############################################################################
 */

/* #############################################################################
 * includes
 */
#include "cpm.h"
#ifdef HAVE_LIBNCURSES
  #include <ncurses.h>
#endif
#ifdef HAVE_LIBNCURSESW
  #include <ncurses.h>
#endif
#ifdef HAVE_TERMIOS_H
  #include <termios.h>
#endif
#include "configuration.h"
#include "general.h"
#include "interface_gui.h"
#include "interface_keys.h"
#include "interface_utf8.h"
#include "interface_xml.h"
#include "listhandler.h"
#include "memory.h"
#include "string.h"
#include "xml.h"


/* #############################################################################
 * global interface variables
 */
struct sKeyEvent
  {
    CDKLABEL*           infobox;
    PROCESSFN           preprocessfunction;
    void*               preprocessdata;
    void*               widget;
    int                 widgettype;
    int                 bindused;
    int                 level;
    int                 selectionid;
    char**              infodata;
    char*               selection;
  };
typedef struct sKeyEvent KEYEVENT;

CDKSCREEN*              cdkscreen;
WINDOW*                 curseswin;
WINDOW*                 statusline = NULL;


/* #############################################################################
 * internal functions
 */
int checkForSecretKey(void);
void clearStatusline(void);
void commentFormat(char** infodata, char* comment);
void drawStatusline(int level);
int getInfodataLength(void);
char* getListtitle(int level);
int keyPreProcess(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogAddEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key);
int guiDialogAddNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogDeleteEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key);
int guiDialogDeleteNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogEditComment(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogEditEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key);
int guiDialogEditNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogHandleKeys(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogHelp(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
void guiDialogOk(int level, char** message);
const char* guiDialogPassphrase(int retry, char* realm);
void guiDialogShowError(const char* headline, const char* message);
int guiDialogTemplateName(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogWrite(EObjectType cdktype, void* object, void* clientdata,
    chtype key);
int guiDialogYesNo(int level, char** message);
int guiDialogYesNoCancel(int level, char** message);
char** guiMessageFormat(char** message);
void guiMessageClear(char** message);
void guiUpdateInfo(CDKLABEL* infobox, char** infodata, int id);
void freshAlphalist(CDKALPHALIST* widget);
int initializeScreen(void);
char* isNodename(KEYEVENT* event);
void updateKeyList(KEYEVENT* event);
RETSIGTYPE resizehandler(int signum);


/* #############################################################################
 *
 * Description    redraw scroll list after key add/remove
 * Author         Kacper Wysocki
 * Date           2010-06-14
 * Arguments      KEYEVENT
 * Return         void
 */

void updateKeyList(KEYEVENT* event)
{
    if (event->widget && event->widgettype == vSCROLL) {
        int counter = keyCount();
        CDKSCROLL* scroll = (CDKSCROLL*) event->widget;
        setCDKScrollItems(scroll, keyGetList(), counter, NONUMBERS);
        eraseCDKScroll(scroll);
        drawCDKScroll(scroll, TRUE);
    }
    return;
}

/* #############################################################################
 *
 * Description    check if we have any secret keys in our keylist
 * Author         Harry Brueckner
 * Date           2005-04-25
 * Arguments      void
 * Return         int 1 if we have our secret key in use, otherwise 0
 */
int checkForSecretKey(void)
  {
    int                 i,
                        j,
                        secret = 0,
                        size;
    char*               key;
    char*               keyptr;

    TRACE(99, "checkForSecretKey()", NULL);

    for (i = keyCount() - 1; i >= 0; i--)
      {
        key = keyGet(i);

        /* we create a copy of the key id string */
        size = strlen(key) + 1;

        /* extract the mail address listed in <...> at the end of the string */
        keyptr = key + size;
        for (j = size; j >= 0; j--, keyptr--)
          {
            if (keyptr[0] == '<')
              { break; }
          }

        switch (gpgIsSecretKey(keyptr))
          {
            case -1:
            case 0:
                break;
            case 1:
                secret = 1;
                break;
          }

        /* if we found a secret key we exit */
        if (secret)
          { break; }
      }

    return secret;
  }


/* #############################################################################
 *
 * Description    clear the statusline window
 * Author         Harry Brueckner
 * Date           2005-08-04
 * Arguments      void
 * Return         void
 */
void clearStatusline(void)
  {
    TRACE(99, "clearStatusline()", NULL);

    if (statusline)
      {
        werase(statusline);
        wrefresh(statusline);
        delwin(statusline);
        statusline = NULL;
      }
  }


/* #############################################################################
 *
 * Description    format the comment field into the infobox
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      char** infodata   - data array to put the data in, starting
 *                                    at entry 2
 *                char* comment     - the comment to format
 * Return         void
 */
void commentFormat(char** infodata, char* comment)
  {
    int                 id,
                        llength = getInfodataLength(),
                        nl,
                        size;
    char*               ptr;
    char*               wstart;

    TRACE(99, "commentFormat()", NULL);

    if (!comment)
      { return; }

    id = 2;
    nl = 0;
    size = 0;
    ptr = wstart = comment;
    while (*ptr)
      {
        if (ptr[0] == '\\' && ptr[1] == 'n')
          {   /* we replace those two chars with a newline */
            nl = 1;
            *ptr += 2;
          }

        if (*ptr == ' ' || nl)
          {   /* we found a word border */
            if ((strlen(infodata[id]) + size > llength &&
                strlen(infodata[id]) > 1))
              {   /* we need a new line, if there is any left */
                if (++id >= config -> infoheight)
                  { break; }
              }

            strStrncat(infodata[id], wstart, size + 1);
            strStrncat(infodata[id], " ", 1 + 1);

            if (nl)
              {
                nl = 0;
                ptr++;
                if (++id >= config -> infoheight)
                  { break; }
              }

            wstart = ptr + 1;
            size = -1;
          }

        ptr++;
        size++;
      }

    if (*wstart &&
        id < config -> infoheight &&
        strlen(infodata[id]) < llength)
      { strStrncat(infodata[id], wstart, llength - strlen(infodata[id]) + 1); }
  }


/* #############################################################################
 *
 * Description    destroy the ncurses screen
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      int line        - line number of the error
 *                char* message   - message to display in case it's an error
 *                                  and we want to exit immediately
 * Return         1 for error, 0 if all is well
 */
void destroyScreen(int line, char* message)
  {
    TRACE(99, "destroyScreen()", NULL);

    /* clear and destroy the statusline */
    clearStatusline();

    /* clear the screen */
    if (cdkscreen)
      { eraseCDKScreen(cdkscreen); }

    /* free the curses and cdk environment */
    if (curseswin)
      {
        delwin(curseswin);
        curseswin = NULL;
      }
    if (cdkscreen)
      {
        destroyCDKScreen(cdkscreen);
        endCDK();

        cdkscreen = NULL;
      }

    if (message)
      {
        fprintf(stderr, _("t-error: %s (%d)\n"), message, line);
        exit(1);
      }
  }


/* #############################################################################
 *
 * Description    display a statusline; this is drawn without the use of CDK
 *                so we can access the last line in the terminal
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      int level     - the level to draw the statusline for
 * Return         void
 */
void drawStatusline(int level)
  {
    int                 i,
                        max_x,
                        max_y;

    TRACE(99, "drawStatusline()", NULL);

    getmaxyx(curseswin, max_y, max_x);
    if (!statusline)
      { statusline = newwin(1, max_x, max_y - 1, 0); }

    wmove(statusline, 0, 0);
    wattron(statusline, A_REVERSE);
    for (i = 0; i < max_x; i++)
      { waddch(statusline, ' '); }

    wmove(statusline, 0, 1);

    wattron(statusline, A_BOLD | COLOR_PAIR(3));
    waddstr(statusline, "^H");
    wattroff(statusline, A_BOLD | COLOR_PAIR(3));
    waddstr(statusline, _(" Help"));

    waddstr(statusline, " | ");

    if (!runtime -> readonly)
      {
        wattron(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, "^A");
        wattroff(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, _(" Add"));

        waddstr(statusline, " | ");

        wattron(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, "^E");
        wattroff(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, _(" Edit"));

        waddstr(statusline, " | ");

        wattron(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, "^O");
        wattroff(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, _(" Comment"));

        waddstr(statusline, " | ");

        wattron(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, "^K");
        wattroff(statusline, A_BOLD | COLOR_PAIR(3));
        waddstr(statusline, _(" Keys"));

        waddstr(statusline, " | ");
      }

    wattron(statusline, A_BOLD | COLOR_PAIR(3));
    waddstr(statusline, "ENTER");
    wattroff(statusline, A_BOLD | COLOR_PAIR(3));
    waddstr(statusline, _(" Choose"));

    waddstr(statusline, " | ");

    wattron(statusline, A_BOLD | COLOR_PAIR(3));
    waddstr(statusline, "ESC");
    wattroff(statusline, A_BOLD | COLOR_PAIR(3));
    if (level <= 1)
      { waddstr(statusline, _(" Exit")); }
    else
      { waddstr(statusline, _(" Back")); }

    wattroff(statusline, A_REVERSE);
    wrefresh(statusline);
  }


/* #############################################################################
 *
 * Description    determine the width of the text for the infobox
 * Author         Harry Brueckner
 * Date           2005-08-04
 * Arguments      void
 * Return         int with the width of the data for the infobox
 */
int getInfodataLength(void)
  {
    int                 max_x,
                        max_y;

    TRACE(99, "getInfodataLength()", NULL);

    getmaxyx(curseswin, max_y, max_x);

#ifdef CDK_VERSION_5
    max_x -= 2;
#else
    max_x -= 5;
#endif
    if (max_x + 1 < STDBUFFERLENGTH)
        return max_x;
    else
        return STDBUFFERLENGTH;
    (void)max_y; // remove unused var warning
  }


/* #############################################################################
 *
 * Description    generate the title of the alphalist
 * Author         Harry Brueckner
 * Date           2005-04-14
 * Arguments      int level   - the level to display to
 * Return         char* containing the 2line title; it must be freed by the
 *                caller
 */
char* getListtitle(int level)
  {
    int                 defaultlevel,
                        hide,
                        i,
                        is_static;
    char*               subtitle;
    char*               templatename;
    char*               title;

    TRACE(99, "getListtitle()", NULL);

    title = memAlloc(__FILE__, __LINE__, 7 + 1);
    strStrncpy(title, "</B/24>", 7 + 1);
    subtitle = memAlloc(__FILE__, __LINE__, 7 + 1);
    strStrncpy(subtitle, "</B/24>", 7 + 1);

    for (i = 1; i <= level - 1; i++)
      {   /* we collect the template tree */
        templatename = xmlInterfaceTemplateGet(i, &is_static);
        title = memRealloc(__FILE__, __LINE__, title,
            strlen(title) + 1,
            strlen(title) + strlen(templatename) + 1 + 1);

        if (i == 1)
          { strStrncat(title, " ", 1 + 1); }
        else
          { strStrncat(title, "/", 1 + 1); }

        strStrncat(title, templatename, strlen(templatename) + 1);
      }

    defaultlevel = listCount(config -> defaulttemplates);
    for (i = 1; i <= level - 1; i++)
      {   /* we collect the data of the tree */
        hide = 0;
        if (i <= defaultlevel)
          {
            templatename = xmlInterfaceTemplateGet(i, &is_static);
            if (!strcmp(templatename, config -> defaulttemplates[i - 1]))
              {   /* this is still the default template name */
                if (!strcmp(config -> defaulttemplatestatus[i - 1], "password"))
                  { hide = 1; }
              }
          }
        if (hide)
          {   /* if it's a password level, we don't show it in the headline */
            templatename = _("password");
          }
        else
          { templatename = xmlInterfaceNodeGet(i); }

        subtitle = memRealloc(__FILE__, __LINE__, subtitle,
            strlen(subtitle) + 1,
            strlen(subtitle) + strlen(templatename) + 1 + 1);

        if (i == 1)
          { strStrncat(subtitle, " ", 1 + 1); }
        else
          { strStrncat(subtitle, "/", 1 + 1); }

        strStrncat(subtitle, templatename, strlen(templatename) + 1);
      }
    title = memRealloc(__FILE__, __LINE__, title,
        strlen(title) + 1,
        strlen(title) + strlen(subtitle) + 1 + 1);
    strStrncat(title, "\n", 1 + 1);
    strStrncat(title, subtitle, strlen(subtitle) + 1);

    memFreeString(__FILE__, __LINE__, subtitle);

    return title;
  }


/* #############################################################################
 *
 * Description    update the infobox when any key is modified in the alphalist
 *                widget
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int keyPreProcess(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    CDKALPHALIST*       list;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 id,
                        nodes;
    char**              nodenames;

    TRACE(99, "keyPreProcess()", NULL);

    list = event -> widget;

    if (event -> preprocessfunction)
      {   /* here we use our ugly hack to keep the preProcessFunction of the
           * real alphalist widget. If we don't do this, we break the
           * quicksearch feature of the alphalist.
           */
        (event -> preprocessfunction)(vALPHALIST, list,
            event -> preprocessdata, key);
      }

    nodenames = xmlInterfaceGetNames();
    nodes = listCount(nodenames);
    xmlInterfaceFreeNames(nodenames);

    id = list -> scrollField -> currentItem;
    if (nodes == 0 ||
        id >= nodes)
      { return 1; }

    if (key == KEY_DOWN)
      {
        if (++id >= nodes)
          { id--; }
      }
    else if (key == KEY_UP)
      {
        if (--id < 0)
          { id = 0; }
      }

    guiUpdateInfo(event -> infobox, event -> infodata, id);

    return 1;
  }


/* #############################################################################
 *
 * Description    re-create the entry list for the given widget (which must be
 *                the 'current node' in the xml interface)
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      CDKALPHALIST* widget  - the widget to update
 * Return         void
 */
void freshAlphalist(CDKALPHALIST* widget)
  {
    int                 nodes;
    char**              nodenames;

    TRACE(99, "freshAlphalist()", NULL);

    nodenames = xmlInterfaceGetNames();
    nodes = listCount(nodenames);

    setCDKAlphalistContents(widget, nodenames, nodes);
    /* TODO: add support to place the cursor at last added/modified item */
#ifdef CDK_VERSION_5
    setCDKScrollCurrentTop(widget -> scrollField, 0);
    setCDKAlphalistCurrentItem(widget, 0);
    drawCDKAlphalist(widget, BorderOf(widget));
#endif

    xmlInterfaceFreeNames(nodenames);
  }


/* #############################################################################
 *
 * Description    dialog to request the passphrase
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      int retry           - number of this retry
 *                const char* realm   - name of the realm the passphrase is for
 * Return         const char* to the passphrase
 */
const char* guiDialogPassphrase(int retry, char* realm)
  {
    CDKENTRY*           entry = NULL;
    char*               data;
    char*               trealm;
    char*               title;

    TRACE(99, "guiDialogPassphrase()", NULL);

    if (strlen(runtime -> passphrase))
      { return runtime -> passphrase; }

    /* we make the realm terminal usable */
    trealm = convert2terminal((xmlChar*)realm);

    title = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
    snprintf(title, STDBUFFERLENGTH,
        _("</B>enter your passphrase (try #%d)<!B>\n%s"),
        retry, trealm);

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        title,
        "",
        A_NORMAL, '_',
        vHMIXED, 60, 0, PASSPHRASE_LENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    if (title)
        memFree(__FILE__, __LINE__, title, STDBUFFERLENGTH);

    /* set the data of the input field */
    setCDKEntry(entry, "", 0, PASSPHRASE_LENGTH, SHOW_BOX);
    setCDKEntryHiddenChar(entry, config -> hidecharacter);

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* add a new entry to the current node */
        strStrncpy(runtime -> passphrase, data, PASSPHRASE_LENGTH);
        runtime -> passphrase[PASSPHRASE_LENGTH] = 0;
      }

    destroyCDKEntry(entry);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(0); }

    return runtime -> passphrase;
  }


/* #############################################################################
 *
 * Description    dialog to add a new encryption key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogAddEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key)
  {
    char*               msgKeyError[] = { NULL, NULL };
    CDKENTRY*           entry = NULL;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    char*               data;

    TRACE(99, "guiDialogAddEncryptionKey()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        _("</B>add a new encryption key<!B>"),
        "",
        A_NORMAL, '_',
        vMIXED, 60, 0, STDSTRINGLENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    /* set the data of the input field */
    setCDKEntry(entry, event -> selection, 0, STDSTRINGLENGTH, SHOW_BOX);

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* add a new entry to the current node */
        if (keyAdd(data))
          { runtime -> datachanged = 1; }
        else
          {
            msgKeyError[0] = _("The key you entered could not be validated.");
            guiDialogOk(event -> level, msgKeyError);
          }
      }

    destroyCDKEntry(entry);

#ifdef CDK_VERSION_5
    /* tell the widget that it has to exit  */
    /* I am not sure if this is a bug in CDK or not, but it can be
     * solved this way. */
    EarlyExitOf((CDKSCROLL*)object) = vESCAPE_HIT;
#endif

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    /* redraw the calling widget */
    updateKeyList(event);

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to add a new entry to the current level
 * Author         Harry Brueckner
 * Date           2005-03-23
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogAddNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    char*               msgCryptMessage[] = { NULL, NULL, NULL };
    CDKENTRY*           entry = NULL;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 is_static;
    char*               data;
    char*               errormsg;
    char*               status = NULL;
    char*               title;

    TRACE(99, "guiDialogAddNode()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    title = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
    snprintf(title, STDBUFFERLENGTH,
        _("</B>add new %s<!B>"),
        xmlInterfaceTemplateGet(event -> level, &is_static));

    if (event -> level &&
        listCount(config -> defaulttemplatestatus) >= event -> level)
      {   /* we create the title with the template name if possible */
        status = config -> defaulttemplatestatus[event -> level - 1];
      }

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        title,
        "",
        A_NORMAL, '_',
        vMIXED, 60, 0, STDSTRINGLENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    memFree(__FILE__, __LINE__, title, STDBUFFERLENGTH);

    /* set the data of the input field */
    setCDKEntry(entry, NULL, 0, STDSTRINGLENGTH, SHOW_BOX);

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* add a new entry to the current node */
        if (strlen(data))
          {
            if (config -> cracklibstatus == CRACKLIB_ON &&
                status &&
                !strcmp(status, "password"))
              {   /* we must check the password */
                errormsg = isGoodPassword(data);
                if (errormsg)
                  {   /* cracklib complains about the password */
                    msgCryptMessage[0] =
                        _("The password you have chosen is unsuitable.");
                    msgCryptMessage[1] = errormsg;
                    guiDialogOk(event -> level, msgCryptMessage);
                  }
              }
            if (xmlInterfaceNodeExists(data))
              { Beep(); }
            else
              {
                xmlInterfaceAddNode(data);
                runtime -> datachanged = 1;
              }

            /* fresh the widget to add the entry */
            freshAlphalist(event -> widget);
          }
      }

    destroyCDKEntry(entry);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to delete the current encryption key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogDeleteEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key)
  {
    char*               msgDeleteNode[] = { NULL, NULL, NULL };
    CDKSCROLL*          scroll;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 counter,
                        length;

    TRACE(99, "guiDialogDeleteEncryptionKey()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;

    scroll = event -> widget;
    counter = scroll -> currentItem;
    if (counter < 0 ||
        counter >= keyCount())
      {
        Beep();
        return 1;
      }

    /* we ask if this key should be deleted */
    length = strlen(keyGet(counter)) + 9 + 1;
    msgDeleteNode[0] = _("Are you sure you want to delete the encryption key");
    msgDeleteNode[1] = memAlloc(__FILE__, __LINE__, length);
    snprintf(msgDeleteNode[1], length, "%s?", keyGet(counter));

    if (guiDialogYesNo(event -> level, msgDeleteNode) == 1)
      {   /* we really want to delete this key */
        keyDelete(counter);
        runtime -> datachanged = 1;
      }

    memFree(__FILE__, __LINE__, msgDeleteNode[1], length);

#ifdef CDK_VERSION_5
    /* tell the widget that it has to exit  */
    /* I am not sure if this is a bug in CDK or not, but it can be
     * solved this way. */
    EarlyExitOf((CDKSCROLL*)object) = vESCAPE_HIT;
#endif
    updateKeyList(event);

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to delete the current node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogDeleteNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    char*               msgDeleteNode[] = { NULL, NULL, NULL };
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 length;
    char*               label;

    TRACE(99, "guiDialogDeleteNode()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    label = isNodename(event);
    if (!label)
      {
        Beep();
        return 1;
      }

    length = strlen(label) + 15 + 1;
    msgDeleteNode[0] = _("Are you sure you want to delete");
    msgDeleteNode[1] = memAlloc(__FILE__, __LINE__, length);
    snprintf(msgDeleteNode[1], length, _("entry %s?"), label);

    if (guiDialogYesNo(event -> level, msgDeleteNode) == 1)
      {   /* we really want to delete this node */
        xmlInterfaceDeleteNode(label);
        runtime -> datachanged = 1;

        /* fresh the widget to remove the entry */
        freshAlphalist(event -> widget);
      }

    memFree(__FILE__, __LINE__, msgDeleteNode[1], length);

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to edit the current node's comment
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogEditComment(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    CDKMENTRY*          mentry;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 id;
    char**              nodenames;
    char*               comment;
    char*               label;

    TRACE(99, "guiDialogEditComment()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    label = isNodename(event);
    if (!label)
      {
        Beep();
        return 1;
      }

    /* get the current comment */
    comment = xmlInterfaceGetComment(label);

    mentry = newCDKMentry(cdkscreen, CENTER, CENTER,
        _("</B>edit comment<!B>"),
        "",
        A_NORMAL, ' ',
        vMIXED, 60, 20, 20, 0,
        SHOW_BOX, SHOW_SHADOW);
    if (!mentry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    setCDKMentry(mentry, comment, -1, SHOW_BOX);
    activateCDKMentry(mentry, (chtype*)NULL);
    if (mentry -> exitType == vNORMAL)
      {
        xmlInterfaceSetComment(label, mentry -> info);
        runtime -> datachanged = 1;

        /* we update the infobox */
        nodenames = xmlInterfaceGetNames();
        id = 0;
        while (nodenames[id])
          {
            if (!strcmp(label, nodenames[id]))
              {
                guiUpdateInfo(event -> infobox, event -> infodata, id);
                break;
              }

            id++;
          }
        xmlInterfaceFreeNames(nodenames);
      }
    destroyCDKMentry(mentry);

    /* free our old comment */
    memFreeString(__FILE__, __LINE__, comment);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to edit the current encryption key
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogEditEncryptionKey(EObjectType cdktype, void* object,
    void* clientdata, chtype key)
  {
    char*               msgKeyError[] = { NULL, NULL };
    CDKSCROLL*          scroll;
    CDKENTRY*           entry = NULL;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 counter;
    char*               data;

    TRACE(99, "guiDialogEditEncryptionKey()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;

    scroll = event -> widget;
    counter = scroll -> currentItem;
    if (counter < 0 ||
        counter >= keyCount())
      {
        Beep();
        return 1;
      }

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        _("</B>edit encryption key<!B>"),
        "",
        A_NORMAL, '_',
        vMIXED, 60, 0, STDSTRINGLENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    /* set the data of the input field */
    setCDKEntry(entry, keyGet(counter), 0, STDSTRINGLENGTH, SHOW_BOX);

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* modify the entry*/
        if (keyChange(counter, data))
          { runtime -> datachanged = 1; }
        else
          {
            msgKeyError[0] = _("The key you entered could not be validated.");
            guiDialogOk(event -> level, msgKeyError);
          }
      }

    destroyCDKEntry(entry);

#ifdef CDK_VERSION_5
    /* tell the widget that it has to exit  */
    /* I am not sure if this is a bug in CDK or not, but it can be
     * solved this way. */
    EarlyExitOf((CDKSCROLL*)object) = vESCAPE_HIT;
#endif

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }
    updateKeyList(event);

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to edit the current node
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogEditNode(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    char*               msgCryptMessage[] = { NULL, NULL, NULL };
    char*               msgRandom[] = { NULL, NULL, NULL, NULL };
    CDKENTRY*           entry = NULL;
    CDKLABEL*           randombox;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 is_static;
    char*               data;
    char*               errormsg;
    char*               label;
    char*               password = NULL;
    char*               status = NULL;
    char*               title;

    TRACE(99, "guiDialogEditNode()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    label = isNodename(event);
    if (!label)
      {
        Beep();
        return 1;
      }

    title = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
    snprintf(title, STDBUFFERLENGTH, _("</B>edit %s<!B>"),
        xmlInterfaceTemplateGet(event -> level, &is_static));

    if (event -> level &&
        listCount(config -> defaulttemplatestatus) >= event -> level)
      {   /* we create the title with the template name if possible */
        status = config -> defaulttemplatestatus[event -> level - 1];
      }

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        title,
        "",
        A_NORMAL, '_',
        vMIXED, 60, 0, STDSTRINGLENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    memFree(__FILE__, __LINE__, title, STDBUFFERLENGTH);

    if (key == '')
      {   /* we have to create a new password */
        msgRandom[0] = _("If it looks like the program is stuck");
        msgRandom[1] = _("move the mouse to create more entropy");
        msgRandom[2] = _("for the random generator.");

        randombox = newCDKLabel(cdkscreen, CENTER, CENTER,
            msgRandom, 3, SHOW_BOX, SHOW_SHADOW);
        if (!randombox)
          { destroyScreen(__LINE__, _("can not create randombox.")); }
        refreshCDKScreen(cdkscreen);

        password = createPassword(config -> passwordlength);

        /* free the infobox */
        destroyCDKLabel(randombox);

        /* redraw the statusline */
        if (statusline)
          { drawStatusline(event -> level); }

        /* set the data of the input field */
        setCDKEntry(entry, password, 0, STDSTRINGLENGTH, SHOW_BOX);
      }
    else
      {   /* set the data of the input field */
        setCDKEntry(entry, label, 0, STDSTRINGLENGTH, SHOW_BOX);
      }

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* modify the entry*/
        if (strlen(data))
          {
            if (config -> cracklibstatus == CRACKLIB_ON &&
                status &&
                !strcmp(status, "password"))
              {   /* we must check the password */
                errormsg = isGoodPassword(data);
                if (errormsg)
                  {   /* cracklib complains about the password */
                    msgCryptMessage[0] =
                        _("The password you have chosen is unsuitable."),
                    msgCryptMessage[1] = errormsg;
                    guiDialogOk(event -> level, msgCryptMessage);
                  }
              }

            if (xmlInterfaceNodeExists(data))
              { Beep(); }
            else
              {
                xmlInterfaceEditNode(label, data);
                runtime -> datachanged = 1;
              }

            /* fresh the widget to change the entry */
            freshAlphalist(event -> widget);
          }
      }

    if (password)
      { memFreeString(__FILE__, __LINE__, password); }

    destroyCDKEntry(entry);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    return 1;
  }


/* #############################################################################
 *
 * Description    dialog to handle the keys to encrypt the database with
 * Author         Harry Brueckner
 * Date           2005-03-30
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogHandleKeys(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    CDKSCROLL*          scroll;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    KEYEVENT            keyevent;
    int                 counter,
                        done = 0,
                        selection;

    TRACE(99, "guiDialogHandleKeys()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    do
      {
        char *keymsg = _("</B>Keys to encrypt the database with<!B>");
        counter = keyCount();
        if(!counter){
           keymsg = _("</B>No keys to encrypt the database with<!B>. Hit ^A to add a key.");
        }

        scroll = newCDKScroll(cdkscreen, CENTER, CENTER, RIGHT,
            LINES * 2 / 3, COLS - 20,
            keymsg,
            keyGetList(), counter,
            NONUMBERS, A_REVERSE, SHOW_BOX, SHOW_SHADOW);
        if (!scroll)
          { destroyScreen(__LINE__, _("can not create scroll list.")); }

        keyevent.infobox = NULL;
        keyevent.infodata = NULL;
        keyevent.preprocessfunction = NULL;
        keyevent.preprocessdata = NULL;
        keyevent.widget = scroll;
        keyevent.widgettype = vSCROLL;
        keyevent.bindused = 0;
        keyevent.level = 0;
        keyevent.selectionid = 0;
        keyevent.selection = NULL;

        /* we bind the keys to the list */
        bindCDKObject(vSCROLL, scroll, '', guiDialogAddEncryptionKey,
            &keyevent);
        bindCDKObject(vSCROLL, scroll, '', guiDialogDeleteEncryptionKey,
            &keyevent);
        bindCDKObject(vSCROLL, scroll, '', guiDialogEditEncryptionKey,
            &keyevent);

        selection = activateCDKScroll(scroll, NULL);
        if (scroll -> exitType == vNORMAL)
          {
            keyevent.selectionid = scroll -> currentItem;
            guiDialogEditEncryptionKey(vSCROLL, scroll, &keyevent, 0);
          }
        else if (scroll -> exitType == vESCAPE_HIT &&
            !keyevent.bindused)
          { done = 1; }

        destroyCDKScroll(scroll);

        /* redraw the statusline */
        if (statusline)
          { drawStatusline(event -> level); }
      }
    while (!done);

    return 1;
    (void) selection; // unused
  }


/* #############################################################################
 *
 * Description    show the help screem
 * Author         Harry Brueckner
 * Date           2005-04-04
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogHelp(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    static char*        msgHelp[] = {
                            NULL, "", /*  0 */
                            NULL, "", /*  2 */
                            NULL, "", /*  4 */
                            NULL, "", /*  6 */
                            NULL, "", /*  8 */
                            NULL, "", /* 10 */
                            NULL, "", "", /* 12 */
                            NULL, "", /* 15 */
                            NULL,     /* 17 */
                            NULL
                            };
    KEYEVENT*           event = (KEYEVENT*)clientdata;

    TRACE(99, "guiDialogHelp()", NULL);

    msgHelp[ 0] = _(" </B>^A<!B> - add a new node to the current one.");
    msgHelp[ 2] = _(" </B>^D<!B> - delete the currently selected node and all its subnodes.");
    msgHelp[ 4] = _(" </B>^E<!B> - edit the currently selected node.");
    msgHelp[ 6] = _(" </B>^H<!B> - this help screen.");
    msgHelp[ 8] = _(" </B>^K<!B> - edit the currently used encryption keys.");
    msgHelp[10] = _(" </B>^N<!B> - edit the name of the current level.");
    msgHelp[12] = _(" </B>^O<!B> - edit the comment of the selected node.");
    msgHelp[13] = _("      (use \\n to add line breaks)");
    msgHelp[15] = _(" </B>^P<!B> - edit the current node and suggest a password.");
    msgHelp[17] = _(" </B>^W<!B> - Write the database to disk.");

    guiDialogOk(event -> level, msgHelp);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    return 1;
  }


/* #############################################################################
 *
 * Description    show a message with only an ok-button to select
 * Author         Harry Brueckner
 * Date           2005-03-31
 * Arguments      int level       - current dialog level
 *                char** message  - message to display, terminated with a NULL
 *                                  line
 * Return         void
 */
void guiDialogOk(int level, char** message)
  {
    static char*        buttons[] = { NULL };
    CDKDIALOG*          question = NULL;
    int                 done = 0,
                        i,
                        lines = 0,
                        nlines,
                        selection;
    char**              display = NULL;
    char**              tmessage;
    char*               ptr;

    TRACE(99, "guiDialogOk()", NULL);

    buttons[0] = _("</B>Ok<!B>");

    while (message[lines])
      { lines++; }

    for (i = nlines = 0; i < lines; i++)
      {
        ptr = message[i];
        while (strlen(ptr) > 72)
          {
            display = memRealloc(__FILE__, __LINE__, display,
                sizeof(char*) * nlines, sizeof(char*) * (nlines + 1));
            display[nlines] = memAlloc(__FILE__, __LINE__, 72 + 1);
            strStrncpy(display[nlines], ptr, 72 + 1);
            ptr += 72;
            nlines++;
          }

        if (strlen(ptr) || !strlen(message[i]))
          {
            display = memRealloc(__FILE__, __LINE__, display,
                sizeof(char*) * nlines, sizeof(char*) * (nlines + 1));
            display[nlines] = memAlloc(__FILE__, __LINE__, strlen(ptr) + 1);
            strStrncpy(display[nlines], ptr, strlen(ptr) + 1);
          }

        nlines++;
      }

    display = memRealloc(__FILE__, __LINE__, display,
        sizeof(char*) * nlines, sizeof(char*) * (nlines + 1));
    display[nlines] = NULL;

    do
      {
        tmessage = guiMessageFormat(display);
        question = newCDKDialog(cdkscreen, CENTER, CENTER,
            tmessage, nlines,
            buttons, 1,
            A_REVERSE,
            TRUE, SHOW_BOX, SHOW_SHADOW);
        guiMessageClear(tmessage);
        if (!question)
          { destroyScreen(__LINE__, _("can not create dialog.")); }

        selection = activateCDKDialog(question, (chtype*)NULL);

        /* if the dialog exited normaly, it's ok to return otherwise we cycle */
        if (question -> exitType == vNORMAL &&
            selection == 0)
          { done = 1; }

        destroyCDKDialog(question);

        /* redraw the statusline */
        if (statusline)
          { drawStatusline(level); }
      }
    while (!done);

    for (i = 0; i < nlines; i++)
      { memFreeString(__FILE__, __LINE__, display[i]); }
    memFree(__FILE__, __LINE__, display, sizeof(char*) * (nlines + 1));

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(level); }
  }


/* #############################################################################
 *
 * Description    show a error message to the user
 * Author         Harry Brueckner
 * Date           2005-04-07
 * Arguments      char* message   - error message to show
 * Return         void
 */
void guiDialogShowError(const char* headline, const char* message)
  {
    static char*        msgError[] = { NULL, NULL, NULL };

    TRACE(99, "guiDialogShowError()", NULL);

    msgError[0] = memAlloc(__FILE__, __LINE__, strlen(headline) + 8 + 1);
    snprintf(msgError[0], strlen(headline) + 8 + 1, "</B>%s<!B>", headline);
    msgError[1] = (char*)message;
    guiDialogOk(0, msgError);
    memFreeString(__FILE__, __LINE__, msgError[0]);
  }


/* #############################################################################
 *
 * Description    dialog to set the current template name
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogTemplateName(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    CDKENTRY*           entry;
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    int                 is_static;
    char*               data;

    TRACE(99, "guiDialogTemplateName()", NULL);

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    entry = newCDKEntry(cdkscreen, CENTER, CENTER,
        _("</B>template name<!B>"),
        "",
        A_NORMAL, '_',
        vMIXED, 60, 0, STDSTRINGLENGTH,
        SHOW_BOX, SHOW_SHADOW);
    if (!entry)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    /* set the data of the input field */
    setCDKEntry(entry, xmlInterfaceTemplateGet(-1, &is_static), 0,
        STDSTRINGLENGTH, SHOW_BOX);

    data = activateCDKEntry(entry, (chtype*)NULL);
    if (entry -> exitType == vNORMAL)
      {   /* modify the entry*/
        if (strlen(data))
          {
            /* set the new template name */
            xmlInterfaceTemplateSet(data);
            runtime -> datachanged = 1;
          }
      }

    destroyCDKEntry(entry);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(event -> level); }

    return 1;
  }


/* #############################################################################
 *
 * Description    ask a question which can be answered with 'yes' or 'no'.
 * Author         Harry Brueckner
 * Date           2005-03-21
 * Arguments      int level       - current dialog level
 *                char** message  - message to display, terminated with a NULL
 *                                  line
 * Return         1 for yes, 0 for no
 */
int guiDialogYesNo(int level, char** message)
  {
    static char*        buttons[] = { NULL, NULL };
    CDKDIALOG*          question = NULL;
    int                 lines = 0,
                        selection,
                        status = 0;
    char**              tmessage;

    TRACE(99, "guiDialogYesNo()", NULL);

    buttons[0] = _("</B>Yes<!B>");
    buttons[1] = _("</B>No<!B>");

    while (message[lines])
      { lines++; }

    tmessage = guiMessageFormat(message);
    question = newCDKDialog(cdkscreen, CENTER, CENTER,
        tmessage, lines,
        buttons, 2,
        A_REVERSE,
        TRUE, SHOW_BOX, SHOW_SHADOW);
    guiMessageClear(tmessage);
    if (!question)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    selection = activateCDKDialog(question, (chtype*)NULL);

    /* if the dialog exited normaly and the 1st button was selected, it's yes;
     * otherwise its always no (same for pressing ESCAPE)
     */
    if (question -> exitType == vNORMAL &&
        selection == 0)
      { status = 1; }

    destroyCDKDialog(question);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(level); }

    return status;
  }


/* #############################################################################
 *
 * Description    write the data to disk
 * Author         Harry Brueckner
 * Date           2005-04-21
 * Arguments      EObjectType cdktype   - type of object
 *                void* object          - the object
 *                void* clientdata      - special data to the bind function
 *                chtype key            - pressed key
 * Return         1 if the key was 'used', otherwise 0
 */
int guiDialogWrite(EObjectType cdktype, void* object, void* clientdata,
    chtype key)
  {
    static char*        msgEncryptError[] = { NULL, NULL, NULL };
    static char*        msgNoKeys[] = { NULL, NULL };
    static char*        msgSaveDone[] = { NULL, NULL };
    KEYEVENT*           event = (KEYEVENT*)clientdata;
    char*               errormsg;

    TRACE(99, "guiDialogWrite()", NULL);

    msgEncryptError[0]  = _("Error encrypting the data.");
    msgNoKeys[0]   = _("You did not specify any encryption keys.");
    msgSaveDone[0] = _("The database has been written to disk.");

    /* we tell our caller, that an external event modified it's data */
    event -> bindused = 1;
    if (runtime -> readonly)
      {   /* if the file is readonly, we just beep */
        Beep();
        return 1;
      }

    if (config -> encryptdata && !keyCount())
      {
        guiDialogOk(event -> level, msgNoKeys);
        return 1;
      }

    if (xmlDataFileWrite(runtime -> dbfile, &errormsg,
        guiDialogPassphrase, guiDialogShowError))
      {   /* error saving the file */
        msgEncryptError[1] = errormsg;
        guiDialogOk(event -> level, msgEncryptError);
      }
    else
      {
        guiDialogOk(event -> level, msgSaveDone);
        runtime -> datachanged = 0;
      }

    return 1;
  }


/* #############################################################################
 *
 * Description    ask a question which can be answered with 'yes', 'no' or
 *                'cancel'.
 * Author         Harry Brueckner
 * Date           2005-04-03
 * Arguments      int level       - current dialog level
 *                char** message  - message to display, terminated with a NULL
 *                                  line
 * Return         1 for yes, 0 for no, 2 for cancel
 */
int guiDialogYesNoCancel(int level, char** message)
  {
    static char*        buttons[] = { NULL, NULL, NULL };
    CDKDIALOG*          question = NULL;
    int                 lines = 0,
                        selection,
                        status = 2;
    char**              tmessage;

    TRACE(99, "guiDialogYesNoCancel()", NULL);

    buttons[0] = _("</B>Yes<!B>");
    buttons[1] = _("</B>No<!B>");
    buttons[2] = _("</B>Cancel<!B>");

    while (message[lines])
      { lines++; }

    tmessage = guiMessageFormat(message);
    question = newCDKDialog(cdkscreen, CENTER, CENTER,
        tmessage, lines,
        buttons, 3,
        A_REVERSE,
        TRUE, SHOW_BOX, SHOW_SHADOW);
    guiMessageClear(tmessage);
    if (!question)
      { destroyScreen(__LINE__, _("can not create dialog.")); }

    selection = activateCDKDialog(question, (chtype*)NULL);

    /* if the dialog exited normaly and the 1st button was selected, it's yes;
     * otherwise its always no (same for pressing ESCAPE)
     */
    if (question -> exitType == vNORMAL)
      {
        switch (selection)
          {
            case 0:   /* yes */
                status = 1;
                break;
            case 1:   /* no */
                status = 0;
                break;
            case 2:   /* cancel */
                status = 2;
                break;
          }
      }

    destroyCDKDialog(question);

    /* redraw the statusline */
    if (statusline)
      { drawStatusline(level); }

    return status;
  }


/* #############################################################################
 *
 * Description    Clear the given message array
 * Author         Harry Brueckner
 * Date           2007-01-24
 * Arguments      char** message  - message array to format, terminated by NULL
 * Return         void
 */
void guiMessageClear(char** message)
  {
    int                 lines = 0;

    TRACE(99, "guiMessageClear()", NULL);

    if (!message)
      { return; }

    /* free all strings */
    while (message[lines])
      {
        memFreeString(__FILE__, __LINE__, message[lines]);
        lines++;
      }

    /* free the array of strings */
    memFree(__FILE__, __LINE__, message, sizeof(char*) * (lines + 1));
  }


/* #############################################################################
 *
 * Description    format the given message to match the current terminal width
 * Author         Harry Brueckner
 * Date           2007-01-24
 * Arguments      char** message  - message array to format, terminated by NULL
 * Return         char** cut off message strings
 */
#define WIDTHDELTA      10
char** guiMessageFormat(char** message)
  {
    int                 columns,
                        i,
                        lines = 0;
    char**              nmessage;

    TRACE(99, "guiMessageFormat()", NULL);

    if (!message)
      { return NULL; }

    /* count strings */
    while (message[lines])
      { lines++; }

    nmessage = memAlloc(__FILE__, __LINE__, sizeof(char*) * (lines + 1));
    nmessage[lines] = NULL;

    columns = getInfodataLength();
    columns = max(WIDTHDELTA * 2, columns);

    for (i = 0; i < lines; i++)
      {
        int             size;

        size = min(columns - WIDTHDELTA + 1, strlen(message[i]) + 1);
        nmessage[i] = memAlloc(__FILE__, __LINE__, size);
        strStrncpy(nmessage[i], message[i], size);
      }

    return nmessage;
  }
#undef WIDTHDELTA


/* #############################################################################
 *
 * Description    initialization of the ncurses screen
 * Author         Harry Brueckner
 * Date           2005-03-17
 * Arguments      void
 * Return         1 for error, 0 if all is well
 */
int initializeScreen(void)
  {
    TRACE(99, "initializeScreen()", NULL);

    curseswin = NULL;
    cdkscreen = NULL;

    /* initialize ncurses */
    clear_screen();
    curseswin = initscr();
    if (!curseswin)
      { return 1; }
    /* initialize cdk */
    cdkscreen = initCDKScreen(curseswin);
    if (!cdkscreen)
      { return 1; }

    /* per default the background should be transparent */
    /* users can modify this behaviour by setting e.g.
     * NCURSES_ASSUMED_COLORS="-1,-1"
     */
    assume_default_colors(-1, -1);

    /* we do not want any ESC delay */
    ESCDELAY = 0;

    /* start color mode */
    initCDKColor();

    /* clear the screen */
    eraseCDKScreen(cdkscreen);

    return 0;
  }


/* #############################################################################
 *
 * Description    update the infobox
 * Author         Harry Brueckner
 * Date           2005-04-05
 * Arguments      CDKLABEL* infobox   - the infobox widget
 *                char** infodata     - the data array for the comment
 *                int id              - node id to display the comment of
 * Return         1 if the key was 'used', otherwise 0
 */
void guiUpdateInfo(CDKLABEL* infobox, char** infodata, int id)
  {
    int                 i,
                        nodes;
    char**              nodenames;
    char*               comment;
    char*               created_by;
    char*               created_on;
    char*               modified_by;
    char*               modified_on;

    TRACE(99, "guiUpdateInfo()", NULL);

    /* we have to fit the comment into the infodata array */
    for (i = 2; i < config -> infoheight; i++)
      {
        infodata[i][0] = ' ';
        infodata[i][1] = 0;
      }

    nodenames = xmlInterfaceGetNames();
    nodes = listCount(nodenames);

    if (nodes)
      {
        comment = xmlInterfaceGetComment(nodenames[id]);
        xmlInterfaceGetCreationLabel(nodenames[id], &created_by, &created_on);
        xmlInterfaceGetModificationLabel(nodenames[id], &modified_by,
            &modified_on);

        if (modified_by && modified_on &&
            strcmp(modified_on, "---"))
          {   /* only if the modification entry exists, we display it */
            snprintf(infodata[0], STDBUFFERLENGTH,
                _(" modified by </B>%s<!B>"), modified_by);
            snprintf(infodata[1], STDBUFFERLENGTH,
                _(" modified on </B>%s<!B>"), modified_on);
          }
        else
          {
            snprintf(infodata[0], STDBUFFERLENGTH,
                _(" created by </B>%s<!B>"), created_by);
            snprintf(infodata[1], STDBUFFERLENGTH,
                _(" created on </B>%s<!B>"), created_on);
          }

        commentFormat(infodata, comment);

        memFreeString(__FILE__, __LINE__, modified_by);
        memFreeString(__FILE__, __LINE__, modified_on);
        memFreeString(__FILE__, __LINE__, created_by);
        memFreeString(__FILE__, __LINE__, created_on);
        memFreeString(__FILE__, __LINE__, comment);

        xmlInterfaceFreeNames(nodenames);
      }

    setCDKLabelMessage(infobox, infodata, config -> infoheight);
    drawCDKLabel(infobox, FALSE);
  }


/* #############################################################################
 *
 * Description    this function handles all user interacitvities
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      void
 * Return         void
 */
void interfaceLoop(void)
  {
    CDKALPHALIST**      listwidget = NULL;
    CDKLABEL*           infobox;
    KEYEVENT**          keyevent = NULL;
    int                 done = 0,
                        id,
                        is_static,
                        level = 1,
                        maxlevel = 0,
                        nodes;
    char**              infodata;
    char**              nodenames;
    char*               quicksearch;
    char*               selection;
    char*               title;

    TRACE(99, "interfaceLoop()", NULL);

    infodata = memAlloc(__FILE__, __LINE__,
        sizeof(char*) * (config -> infoheight + 1));
    for (id = 0; id < config -> infoheight; id++)
      {
        infodata[id] = memAlloc(__FILE__, __LINE__, STDBUFFERLENGTH);
        snprintf(infodata[id], STDBUFFERLENGTH, "%*s",
            getInfodataLength(), "");
      }
    infodata[id] = NULL;

#ifdef CDK_VERSION_5
    infobox = newCDKLabel(cdkscreen, LEFT, LINES - config -> infoheight - 3,
        infodata, config -> infoheight, SHOW_BOX, SHOW_SHADOW);
#else
    infobox = newCDKLabel(cdkscreen, LEFT, BOTTOM,
        infodata, config -> infoheight, SHOW_BOX, SHOW_SHADOW);
#endif
    if (!infobox)
      { destroyScreen(__LINE__, _("can not create infobox.")); }

    drawCDKLabel(infobox, FALSE);

    drawStatusline(level);

    xmlInterfaceNodeDown(NULL);

    while (!done)
      {
        if (maxlevel < level)
          {   /* reallocate as many pointers as needed at the moment */
            listwidget = memRealloc(__FILE__, __LINE__,
                listwidget,
                maxlevel * sizeof(CDKALPHALIST*),
                level * sizeof(CDKALPHALIST*));
            listwidget[level - 1] = NULL;
            keyevent = memRealloc(__FILE__, __LINE__, keyevent,
                maxlevel * sizeof(KEYEVENT*),
                level * sizeof(KEYEVENT*));
            keyevent[level - 1] = NULL;

            maxlevel = level;
          }

        id = level - 1;
        if (!listwidget[id])
          {
            title = xmlInterfaceTemplateGet(level, &is_static);
            quicksearch = memAlloc(__FILE__, __LINE__,
                strlen(title) + 11 + 1);
            snprintf(quicksearch, strlen(title) + 11 + 1,
                " </B>%s<!B>: ", title);
            title = getListtitle(level);

            /* get the names of the current nodes children */
            nodenames = xmlInterfaceGetNames();
            nodes = listCount(nodenames);

            listwidget[id] = newCDKAlphalist(cdkscreen, RIGHT, 0,
#ifdef CDK_VERSION_5
                LINES - infobox -> boxHeight - 1, 0,
#else
                LINES - infobox -> boxHeight - 2, 0,
#endif
                title,
                quicksearch,
                nodenames,
                nodes,
                '_',
                A_REVERSE, SHOW_BOX, SHOW_SHADOW);
            if (!listwidget[id])
              { destroyScreen(__LINE__, _("can not create alpha list.")); }

            xmlInterfaceFreeNames(nodenames);
            memFreeString(__FILE__, __LINE__, quicksearch);
            memFreeString(__FILE__, __LINE__, title);
          }
        else
          { drawCDKAlphalist(listwidget[id], FALSE); }

        /* we store our widget handler */
        if (!keyevent[id])
          {
            keyevent[id] = memAlloc(__FILE__, __LINE__,
                sizeof(KEYEVENT));
            keyevent[id] -> infobox = infobox;
            keyevent[id] -> infodata = infodata;
            keyevent[id] -> preprocessfunction = NULL;
            keyevent[id] -> preprocessdata = NULL;
            keyevent[id] -> widget = listwidget[id];
            keyevent[id] -> widgettype = vALPHALIST;
            keyevent[id] -> bindused = 0;
            keyevent[id] -> level = level;
            keyevent[id] -> selectionid = 0;
            keyevent[id] -> selection = NULL;

            /* we catch all keys and keep the old preprocess function;
             * this is necessary since the pre- and post-process functions are
             * not called one after another but simply overwrite the old one!
             */
            keyevent[id] -> preprocessfunction =
                cpmObjOf(listwidget[id] -> entryField) -> preProcessFunction;
            keyevent[id] -> preprocessdata =
                cpmObjOf(listwidget[id] -> entryField) -> preProcessData;

            setCDKAlphalistPreProcess(listwidget[id], keyPreProcess,
                keyevent[id]);
         }

        /* we update the infobox */
        guiUpdateInfo(infobox, infodata, 0);
        /* we redraw the statusline to update exit/back comments */
        drawStatusline(level);

        /* we bind the keys to the dialog */
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogAddNode,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogDeleteNode,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogEditNode,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogHelp,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogTemplateName,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogEditComment,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogEditNode,
            keyevent[id]);
        bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogWrite,
            keyevent[id]);
        if (config -> encryptdata)
          {
            bindCDKObject(vALPHALIST, listwidget[id], '', guiDialogHandleKeys,
                keyevent[id]);
          }


        selection = activateCDKAlphalist(listwidget[id], NULL);

        /* and update the selection */
        keyevent[id] -> selection = selection;

        if (listwidget[id] -> exitType == vESCAPE_HIT)
          {   /* if the user exits with ESCAPE, we move one level up; if it's
               * the highest level, we exit
               */
            if (--level == 0)
              { done = 1; }

            /* destroy the alpha list */
            destroyCDKAlphalist(listwidget[id]);
            listwidget[id] = NULL;
            memFree(__FILE__, __LINE__, keyevent[id], sizeof(KEYEVENT));
            keyevent[id] = NULL;
            xmlInterfaceNodeUp();
          }
        else if (listwidget[id] -> exitType == vNORMAL)
          {   /* something has been selected */
            if (xmlInterfaceNodeDown(selection))
              {   /* in case it's not an existing entry, we use the currently
                   * selected entry
                   */
                selection = isNodename(keyevent[id]);

                if (!xmlInterfaceNodeDown(selection))
                  {
                    xmlInterfaceTemplateGet(++level, &is_static);
                    if (config -> templatelock && !is_static)
                      {   /* if it's not a static template we can't go there */
                        xmlInterfaceNodeUp();
                        Beep();
                        level--;
                      }
                  }
              }
            else
              {
                xmlInterfaceTemplateGet(++level, &is_static);
                if (config -> templatelock && !is_static)
                  {   /* if it's not a static template we can't go there */
                    Beep();
                    level--;
                  }
              }
          }
      }

    /* we free our infobox data */
    for (id = 0; id < config -> infoheight; id++)
      { memFree(__FILE__, __LINE__, infodata[id], STDBUFFERLENGTH); }
    memFree(__FILE__, __LINE__,
        infodata, sizeof(char*) * (config -> infoheight + 1));

    /* free the infobox */
    destroyCDKLabel(infobox);

    /* free the widget memory */
    if (keyevent)
      { memFree(__FILE__, __LINE__, keyevent, maxlevel * sizeof(KEYEVENT*)); }
    if (listwidget)
      {
        memFree(__FILE__, __LINE__,
            listwidget, maxlevel * sizeof(CDKALPHALIST*));
      }
  }


/* #############################################################################
 *
 * Description    find the node we want to edit/delete by the current selection
 * Author         Harry Brueckner
 * Date           2005-03-29
 * Arguments      KEYEVENT* event   - event from which we need the entry field
 * Return         char* to the label which was entered, if it's not found NULL
 *                is returned
 */
char* isNodename(KEYEVENT* event)
  {
    CDKALPHALIST*       list;
    int                 id;
    char*               label = NULL;

    TRACE(99, "isNodename()", NULL);

    list = event -> widget;
    if (list -> scrollField -> listSize > 0)
      {
        id = list -> scrollField -> currentItem;
        if (id >= 0)
          { label = list -> list[id]; }
      }

    return label;
  }


/* #############################################################################
 *
 * Description    signal handler to catch SIGWINCH
 * Author         Harry Brueckner
 * Date           2005-08-04
 * Arguments      int signum  - signal number
 * Return         void
 */
RETSIGTYPE resizehandler(int signum)
  {
    int                 ch;

    if (signum == SIGWINCH)
      {   /* we must resize our windows */
        ch = CTRL('L');
        ioctl(0, TIOCSTI, &ch);
      }
  }


/* #############################################################################
 *
 * Description    user interface function which loops as long as the user does
 *                not exit
 * Author         Harry Brueckner
 * Date           2005-03-21
 * Arguments      void
 * Return         void
 */
void userInterface(void)
  {
    static char*        msgEncryptError[] = { NULL, NULL, NULL };
    static char*        msgNoKeys[] = { NULL, NULL };
    static char*        msgNoLock[] = { NULL, NULL, NULL, NULL };
    static char*        msgNoSecretKey[] = { NULL, NULL, NULL, NULL, NULL, NULL };
    static char*        msgNoUnlock[] = { NULL, NULL, NULL };
    static char*        msgQuit[] = { NULL, NULL };
    static char*        msgReadOnly[] = { NULL, NULL, NULL };
    static char*        msgRemoveLock[] = { NULL, NULL, NULL };
    static char*        msgUpdate[] = { NULL, NULL };
    int                 done,
                        i,
                        repeat,
                        savemessages;
    char**              msgSaveData;
    char*               errormsg;
    char*               errormsg2;

    TRACE(99, "userInterface()", NULL);

    msgEncryptError[0]  = _("Error encrypting the data.");
    msgNoKeys[0]        = _("You did not specify any encryption keys.");
    msgNoKeys[1]        = _("Quit without saving?");
    msgNoLock[0]        = _("The database file could not be opened exclusively.");
    msgNoLock[1]        = _("The file is opened in read-only mode and");
    msgNoLock[2]        = _("you can not modify any data at the moment.");
    msgNoSecretKey[0]   = _("</B>warning<!B>");
    msgNoSecretKey[1]   = _("You did not specify any of your secret keys to");
    msgNoSecretKey[2]   = _("encrypt the database.");
    msgNoSecretKey[3]   = _("</B>You won't be able to read this file yourself!<!B>");
    msgNoSecretKey[4]   = _("Do you want to continue?");
    msgNoUnlock[0]      = _("The lockfile could not be removed.");
    msgNoUnlock[1]      = _("You will have to manually remove it.");
    msgQuit[0]          = _("Do you really want to quit?");
    msgReadOnly[0]      = _("The database file is read-only.");
    msgReadOnly[1]      = _("You can not modify any data at the moment.");
    msgRemoveLock[0]    = _("A lockfile exists for this database.");
    msgRemoveLock[1]    = _("Do you want to remove the lockfile?");
    msgUpdate[0]        = _("The database was updated to the latest version.");

    if (initializeScreen())
      { destroyScreen(__LINE__, _("can not initialize the screen.")); }

    if (!initUTF8Encoding(config -> encoding))
      {
        destroyScreen(__LINE__,
            _("gui failed to initialize the character encoding."));
      }

    if (xmlDataFileRead(runtime -> dbfile, &errormsg, guiDialogPassphrase,
        guiDialogShowError))
      {
        if (!errormsg)
          { errormsg = _("file read error."); }
        destroyScreen(__LINE__, errormsg);
      }

    if (runtime -> readonly)
      {   /* if we are already in read-only mode we must not create a lock
           * file at all
           */
      }
    else
      {   /* if we don't get a lock, we run in read-only mode */
        repeat = 1;
        while (repeat)
          {
            switch (fileLockCreate(runtime -> dbfile, "lock", &errormsg))
              {
                case 0:     /* lockfile created */
                    runtime -> lockfilecreated = 1;
                    repeat = 0;
                    break;
                case -1:    /* file already exists */
                    if (guiDialogYesNo(0, msgRemoveLock))
                      {   /* remove the lockfile manually */
                        fileLockRemove(&errormsg2);
                        break;
                      }
                case 1:     /* lockfile creation failed */
                    if (errormsg)
                      {   /* in case there was an error message from
                           * fileLockOpen(), the string must be freed
                           */
                        memFree(__FILE__, __LINE__, errormsg, STDBUFFERLENGTH);
                      }

                    runtime -> readonly = 1;
                    runtime -> datachanged = 0;
                    guiDialogOk(0, msgNoLock);

                    repeat = 0;
                    break;
              }
          }
      }

    /* if the database was updated, we inform the user */
    if (runtime -> updatestatus)
      { guiDialogOk(0, msgUpdate); }

    if (!runtime -> readonly &&
        isReadonly(runtime -> dbfile))
      {   /* if the file is read-only, we tell the user */
        runtime -> readonly = 1;
        runtime -> datachanged = 0;
        guiDialogOk(0, msgReadOnly);
      }

    /* we catch terminal resize events */
    signal(SIGWINCH, resizehandler);

    do
      {   /* we loop until the user really wants to quit */
        interfaceLoop();

        if (config -> asktoquit)
          { done = guiDialogYesNo(0, msgQuit); }
        else
          { done = 1; }

        if (done &&
            config -> encryptdata &&
            !keyCount())
          {
            done = guiDialogYesNo(0, msgNoKeys);
            if (done)
              { runtime -> datachanged = 0; }
          }

        if (done &&
            config -> encryptdata &&
            keyCount() &&
            !checkForSecretKey())
          { done = guiDialogYesNo(0, msgNoSecretKey); }

        if (done &&
            runtime -> datachanged &&
            !runtime -> readonly)
          {   /* if we changed data, we ask if the data should really be
               * saved.
               */
            savemessages = keyCount() + 4;
            msgSaveData = memAlloc(__FILE__, __LINE__,
                sizeof(char*) * savemessages);
            for (i = 0; i < savemessages; i++)
              { msgSaveData[i] = NULL; }

            msgSaveData[0] = _("</B>Data has been modified<!B>");
            msgSaveData[1] = _("Do you want to save the database encrypted for these keys?");
            msgSaveData[2] = "";

            for (i = 0; i < savemessages - 4 && i < 10; i++)
              { msgSaveData[i + 3] = keyGet(i); }

            switch (guiDialogYesNoCancel(0, msgSaveData))
              {
                case 0:   /* no */
                    runtime -> datachanged = 0;
                    break;
                case 1:   /* yes */
                    break;
                case 2:   /* cancel */
                    done = 0;
                    break;
              }

            memFree(__FILE__, __LINE__, msgSaveData,
                sizeof(char*) * savemessages);
          }

        if (done &&
            runtime -> datachanged &&
            !runtime -> readonly)
          {   /* save our data */
            if (xmlDataFileWrite(runtime -> dbfile, &errormsg,
                guiDialogPassphrase, guiDialogShowError))
              {   /* error saving the file */
                msgEncryptError[1] = errormsg;
                guiDialogOk(0, msgEncryptError);
                done = 0;
              }
          }
      }
    while (!done);

    /* we only remove the lockfile, if we have created it before */
    if (runtime -> lockfilecreated &&
        fileLockRemove(&errormsg))
      {   /* we could not remove the lock */
        guiDialogOk(0, msgNoUnlock);
      }

    destroyScreen(__LINE__, NULL);
  }


/* #############################################################################
 */

