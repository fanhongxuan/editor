# editor
A GUI Editor like emacs

ALT+F4: quit

CTRL+F: (Find)find text in current active file.

CTRL+B: (Buffer)switch the opened file, or create a new file.

CTRL+S: (Save)Save current file.

CTRL+X:

CTRL+C:

CTRL+V: cut/copy/paste

CTRL+W: (Workspace)show workspace mini-frame, use mouse right click to add/del dir, update database manually.
        use up/down/left/right to browser the tree.

CTRL+E: (Explorer)show explorer mini-frame, use up/down/left/right to browser the tree.

CTRL+1: Hide all the mini-buffer.

CTRL+K: (Kill) kill active buffer.

ALT+O: (Open)find files, if explorer mini-buffer is show, find file in the current selected dir in explorer mini-buffer.
        if workspace mini-buffer is show, find file in all dir list in workspace.

ALT+F: (Find) Find text in dir, if explorer mini-buffer is show, find file in the current select dir in explorer mini-buffer.
        if workspace mini-buffer is show, find text in all dir list in workspace

ALT+R: (Reference) find the reference of current selected text in workspace.

ALT+T: (Text) find text of current selected text in workspace.Open)find files, if explorer mini-buffer is show, find file in the current selected dir in explorer mini-buffer.
        if workspace mini-buffer is show, find file in all dir list in workspace.

ATL+>: (Goto) find the defination of current selected text in workspac

ALT+S: (Symbol) show the symbol list of current file.

# How to compile:

1) Prepare:

   Need to install g++, make, autoconf, first.

   change to the root dir of editor:

   mkdir -p build/linux/ext

   mkdir -p build/linux/symboldb

   mkdir -p import/wxWidgets/lib/linux/
 
2) Download and compile wxWidgets:

   Download wxWidgets from http://www.wxwidgets.org/downloads/

   config:

       ./configure --enable-monolithic --disable-debug_flag --disable-shared

   Change ScintillaBase.cxx in src/stc/

    search widthLB = Platform::Minimum(widthLB, aveCharWidth*maxListWidth);

    change it to widthLB = Platform::Maximum(widthLB, aveCharWidth*maxListWidth);

   compile:

        make -j8

   copy the include file to the target dir:

        cp include/ -rf ../../editor/import/wxWidgets/

   copy the lib file to the target dir:

        cp -rf lib/* ../import/wxWidgets/lib/linux/

3) Compile global and install global:

   cd import/global/

   tar zvxf global-6.6.2.tar.gz

   cd global-6.6.2

   ./configure

   make -j8

   mkdir -p ../../../build/linux/ext/

   cp gtags/gtags ../../../build/linux/ext/

   cp global/global ../../../build/linux/ext/

4) Compile ctags and install ctags

   download ctags from https://github.com/fanhongxuan/ctags

   unzip the code,

   find and copy libglodb.a libgloutil.a from the directroy of global which is build previously to db/

   ./configure

   make -j8

   copy ctags to the build/linux/ext dir of editor code tree.

5) compile ag:

   cd import/ag/

     tar zvxf the_silver_searcher-2.2.0.tar.gz

     cd the_silver_searcher-2.2.0

     ./configure

     make -j8

     cp ag ../../../build/linux/ext/

6) compile editor:

   chang to the root dir of editor, 

   mkdir -p import/libdb/lib/linux/

   find and and copy libglodb.a libgloutil.a from the directory of global which is build previously to 

   import/libdb/ib/linux/

   make clean

   make -j8

   ce is generated in build/linux/   
