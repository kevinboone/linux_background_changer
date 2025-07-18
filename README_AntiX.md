# AntiX Linux support

I would like to be able to use LBC with AntiX Linux, but it's fiddly.  At the
time of writing (July 2025) AntiX does not provide a command-line utility to
set the wallpaper, that works across all the different session types it
supports. There _is_ a script at `/usr/local/bin/desktop-session-wallpaper`
that parses a user configuration file, which is set by a graphical
configuration utility. So you could use this as the basis for a script that you
could pass to LBC:

    $ lbc --method cmd --cmd /path/to/my/script...

It appears that if you're using the (default) 'zzz' session, the commands to
set the wallpaper are:

    zzzfm --set-wallpaper "$1"
    feh --bg-fill "$1"

`zzzfm` sets the background on the desktop file manager, while `feh` sets the
root window. As I understand it, you need to set the wallpaper in both places
if you're using the Conky desktop decorator in transparent mode, because Conky
can read the root window, but not the file manager. 

BUT...

Even if you do all this, it still won't necessarily work, because `zzzfm`
doesn't have any command-line arguments to set the image fill type.  It always
stretches the image in both directions to fill the entire screen. This isn't
just a problem with LBC: using images that don't match the screen aspect ratio
fails with the AntiX graphical utility as well.

This is a known problem in AntiX, but it seems only to have been reported in
May 2025, so there is no fix yet. Using Antix with sessinon types other than
`zzz` might not have this problem.


