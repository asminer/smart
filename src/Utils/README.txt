
Low-level building block utilities.

File              Description
===============   ============================================================
init_opts         Switchable message and output stream option initialization

initializer       For decentralized initialization of static items.
                  Tracks and manages dependencies.

library           Register external libraries; so we can display
                  copyright and other info.

location          Filename and line number, or other special locations.

messages          Error, warning, reporting, and debugging messages.

outstream         Thin wrapper around std::ostream, with nice mechanism
                  for redirecting output.  Also helper utilities.

sigman            Manages termination signals (e.g., to catch ctrl-c
                  and cleanly exit)

splay             Splay trees of shared object pointers

strings           Strings as shared objects (reference counted)

textfmt           Text formatter, used for online documentation

===============   ============================================================

test_init         Tests the initializer implementation
test_splay        Tests the splay implementation
