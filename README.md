line chat
=========

lchat (line chat) is a line oriented front end for for ii-like chat programs.
It handles the input from keyboard and output file in parallel.  Thus, you are
able to type new messages while new chat lines arriving.  Its main focus is on
usability and simplicity.

![lchat](/lchat.png)

Programs you can use lchat as a front end for:

 * http://tools.suckless.org/ii/
 * http://ratox.2f30.org/
 * http://github.com/younix/sj/
 * http://23.fi/jj/

Requirements
------------

 * [libutf](https://git.suckless.org/sbase)
 * tail(1)
 * grep(1)

TODO
----

 * support for UTF-8
 * fix: cursor positions in cases of line wrapping
 * add input history
 * split slackline as an extra project
 * add support for vi/emacs mode
