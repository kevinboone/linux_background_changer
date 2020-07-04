# LBC (Linux background changer)

A wallpaper changer for various Linux desktop environments.

Version 2.0a

## What is this?

LBC is a simple utility that runs quietly in the background and,
at regular intervals, instructs the Linux desktop environment to change the
background (wallpaper) image. LBC is more flexible than the mechanisms
built into most desktops, for a number of reasons.

- LBC allows recursive directory searches, or arbitrary lists of files 
  and directories, or any combination of these;
- it allows some control over what kind of images to include. For example,
  you can avoid putting potrait-orientation images on a landscape screen;
- it uses a simple, text-based configuration file, or just command-line
  options.

## Example

    $ lbc --method gnome2 --width 1024 --aspect landscape $HOME/Pictures/

Use images in `$HOME/Pictures` and all its subdirectories,
that are in landscape aspect
ratio, at least 1024 pixels wide. Use the `gnome2` background method.
Change interval remains at the default of two minutes. Up to the
default number of images (1000) will be included.

## Dependencies

### Compile-time dependencies

Just the development headers for `libjpeg`:

    # apt-get install jpeglib-dev

### Run-time dependencies

To support _all_ the various background-changing methods, you will
need:

- `xview` (`apt-get install xloadimage`) (for the `xview` method)
- `xfconf-query` (for Xfce4 support)
- `gsettings` (for Gnome 3)
- `gconftool-2` (for Gnome 2)

Of course, a particular system will probably only need one of these.
In practice, the relevant binaries are usually part of 
the desktop environment, and you shouldn't need to install anything
extra.

## Building 

Assuming that the compile-time dependencies are available, it's just
a matter of:

    $ make
    $ sudo make install

Note that I wrote LBC specifically for Linux, and intend it to be 
compiled using `gcc`. It uses `gcc`-specific C library extensions.

## Command-line options

*-a,--aspect={landscape|portrait|any}*

Include images with the specified aspect ratio. The default is 'any'.
Note that any image whose width is greater than its height, even by one
pixels, is "landscape".

*-d,--dirs={dir1:dir2...}*

A colon-separated list of directories to include. This option 
is an alternative
to listing directories or files as plain arguments to the program.
The result is the same, but this option can be used in 
a configuration file.

The list can, in fact, include individual files if necessary.

*--dual*

Enable different images on dual monitors, if the desktop supports it.
`lbc` shows a warning if this feature is enabled with a background
change method that does not support it. At present, I believe Xfce4 is
the only supported desktop that has this feature.

*-f,--foreground*

Run LBC in the foreground, attached to console. This feature is for debugging
-- when run in the background, no logging is produced after all the initial
checks are made. In normal circumstances, LBC detaches from the terminal
after ensuring that image files are available, and no further logging
is produced.

*-h,--height={pixels}*

Include only images of at least the specified height 
(see Limitations section below).

*-i,--interval={seconds}*

Sets the time interval between background changes. The default is 120 
seconds. 

*--log-level={0..4}*

Sets the amount of logging from fatal errors only (0), to a huge amount
of trace logging (4). The default level is 1. Note that most logging will 
only be seen with the `--foreground` option. Log levels 2 or higher
will probably not be comprehensible except when examined along side the 
program's source code.

*--max-files=N*

Limit the number of files stored by the program. If this limit is reached,
LBC will show a warning message. The default value is 1000; this can
safely be increased by a factor of 10-100 on most systems, should the
need arise.

*-m,--method={name}*

Sets the background changing method. See the section 'Background change
methods' for more information. The default is `gnome-shell`.

*-n,--next*

Signals a running instance of LBC switch to the next background image.

*-p,--prev*

Signals a running instance of LBC to switch to the previous background image.

*-s,--stop*

Shut down an instance of the program running in the background.

*-v,--version*

Show version number, and exit

*-w,--width={pixels}*

Include only images of at least the specified width
(see Limitations section below).


## Background change methods

### gnome-shell 

This is the default. With this method, the program uses the command
`gsettings set org.gnome.desktop.background picture-uri` to change
the background. This should work on any Gnome 3 system using `gnome-shell` as
the desktop manager, including Wayland-based systems.

### gnome2

This method uses the command `gconftool-2 --set 
/desktop/gnome/background/picture_filename` This should work on Gnome 2
and systems derived from it.

### xview 

This method uses the command `xview --onroot`. Naturally, it will only
work if `xview` is installed, and if the X root window is visible. The
root window is not normally visible on Gnome and similar desktop setups, 
because
the desktop draws over it. However, it can sometimes be made visible
by disabling desktop painting. This method is mostly suitable for very
minimal X set-ups using old-fashioned window managers.

### xfce4 

This method uses `xfconf-query` to set the backgrounds on up to two
monitors. In principle, Xfce4 allows different backgrounds on each virtual
desktop,
but this program does not make use of that feature. However, it can
put different images on dual monitors.

### cmd 

This method executes a user-supplied command, provided by the `--cmd`
argument. The command will be executed with either one or two arguments; 
if `--dual` is set, there will be two arguments, one for each
image filename. Otherwise, there will be one argument. The command can carry out
any operation. Anything it produces to standard out or standard error will be
visible in foreground mode, otherwise the output is lost. 

## Technical notes

### Configuration file

LBC reads `/etc/lbc.rc` and `$HOME/.lbc.rc`, with the latter taking 
precedence. Any of the long-form command-line options can be used
in the configuration file, in the form 

    option=value

A sample configuration file is in the `samples/` directory but, of course,
this will need to be adjusted to suit a particular installation.


### Starting when the desktop starts

Because there are so many different desktop configurations in use, it's
impossible to give specific details for configuring LBC to
auto-start. In many systems, a program can be started along with the
desktop by creating a `.desktop` file in `$HOME/.config/autostart`.
The `samples/` directory has an example desktop file for this purpose, 
but it will need
to be adjusted to suit the installation.

### Signals 

`lbc` traps SIGUSR1 and SIGUSR2 signals. These move to the next and
previous images, respectively. SIGINT causes `lbc` to shut down cleanly.
`lbc` will take will take up to a second to respond to these signals.

### Locking

`lbc` writes its process ID into a file `$HOME/.lbc.lck`. So long
as it's running, `lbc` will hold an advisory lock on this file. 
This prevents multiple LBC processes being created for the same user,
and also allows one instance to signal another (e.g., when using
the `--next` option). If LBC is killed without being shut down cleanly,
or the system crashes, the lock file might be left behind, and have to
be deleted manually.

### Randomization

LBC displays images in random order. However, images are randomized
at start time, so moving forward or backward through the list of
images will produce consistent results.

### Specifying files rather than directories

LBC is happy to be given a specific list of files, rather than
directories. There's no limit (other than the shell) on the 
number of files, or the number of directories they come from.

There is thus a difference between:

   lbc $HOME/Pictures

and

   lbc $HOME/Pictures/\*.jpg

-- the first example will recursively descend the `Pictures/`
directory, and include all images it finds. The second will only
include images in the top level of the `Pictures/` directory, because
only these will match the `*.jpg` pattern.

Which method is preferable depends on the organization of the images.

### Memory

LBC stores the complete list of image filenames in memory. Worse, 
it stores those filenames with wide-character representation.
A list of a thousand files could consume about half a megabyte of
memory. 
This is a nugatory sum in a modern desktop system, or even a Raspberry Pi.
However, there is a `--max-files` option to limit the memory
usage if necessary -- or to increase it if the circumstances 
allow.

## Limitations

At present image size/aspect checks only work on JPEG files. It would be
easy enough to extend it to cover other image types -- at the cost
of increasing the number of dependencies the program has -- but I don't
have enough images of any other type to test it works properly.

If a directory to be included is actually a symbolic link to
a directory, its name must be followed by a forward-slash ('/'). 
The reason is rather technical, and too boring to explain in detail.

LBC may well conflict with whatever background changer is built into
the desktop environment, unless it's disabled. Whatever facility
the desktop provides to select images will, of course, not
work with LBC.

## Changes from previous version 

Version 2.0 expands directories recursively by default. Previous
versions required recursion to be enabled explicitly.

Because recursive expansion is the default, Version 2.0 has a configurable
file limit.

Version 2.0 has facilities to examine certain image files, and only
include those that match certain criteria. It is possible, for example,
to include only landscape-orientation images for a landscape screen.

Version 2.0 supports a configuration file, as well as explicit configuration
using the command line.

## Version history

<b>Version 0.0.1, October 2013</b><br/>
First release
<p/>

<b>Version 0.0.2, April 2014</b><br/>
Added dual-monitor support
<p/>

<b>Version 0.0.3, November 2016</b><br/>
Re-badged from "gnome-background-changer" and generally tidied up
<p/>

<b>Version 0.0.4, June 2020</b><br/>
Made Xfce4 support more robust, by searching for specific `last-image`
configuration settings, rather than assuming them. These settings seem
surprisingly variable between Xfce4 releases.
<p/>

<b>Version 2.0a, July 2020</b><br/>
Complete reimplementation. Introduced size/aspect ratio filters.
<p/>



