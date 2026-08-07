/* aaserver_os9_compat.c -- classic-Mac (CFM) shim for the AAServer OS 9 build.
 * Add this file to the CodeWarrior project.
 *
 * libSDL references SetDialogTimeout (Appearance modal-dialog timeout). At
 * runtime OS 9 has it in InterfaceLib, but linking CW's DialogsLib stub to
 * satisfy it makes the app import a separate shared library "MacDialogsLib"
 * that does NOT exist as a loadable fragment on classic OS 9 -- the CFM loader
 * then fails the whole app at launch. Provide a harmless local no-op instead
 * and DON'T link DialogsLib. AAServer never opens the modal dialogs SDL would
 * call this for.
 *
 * (putenv/_exit, which the game's compat file also shims, are already provided
 * by this project's MSL, so they are intentionally NOT redefined here.)
 *
 * Types come from the MacHeaders prefix that the target's Prefix File
 * (AAServer_OS9_Prefix.h) force-includes into every translation unit.
 */
pascal OSStatus SetDialogTimeout(DialogRef inDialog, DialogItemIndex inButtonToPress,
                                 UInt32 inSecondsToWait)
{
    (void)inDialog;
    (void)inButtonToPress;
    (void)inSecondsToWait;
    return noErr;
}
