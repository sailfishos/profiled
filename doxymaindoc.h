
/******************************************************************************
** This file is part of profile-qt
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
** Contact: Sakari Poussa <sakari.poussa@nokia.com>
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer. Redistributions in
** binary form must reproduce the above copyright notice, this list of
** conditions and the following disclaimer in the documentation  and/or
** other materials provided with the distribution.
**
** Neither the name of Nokia Corporation nor the names of its contributors
** may be used to endorse or promote products derived from this software 
** without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

/*
 * Profile API Doxygen main page
 */

/**
@mainpage

@section introduction Introduction

Profile service is a new subsysten that consists of two parts:
- profiled - daemon process that keeps track of current profile settings
- libprofile - client library for accessing the profile data.

@section architecture Architecture

High level view to system utilizing profile service:
<ol>
<li> Profile daemon supports any number of clients
<li> Application code includes libprofile.h
<li> Application binary is linked against libprofile.so
<li> Libprofile utilizes D-Bus to communicate with the server
<li> Configuration data files define default profile values
<li> Profile daemon accepts request and sends signals on dbus
<li> Changes to profile values are stored to a file
<li> As is the name of currently active profile
</ol>

ASCII art diagram of the above:

@code
     +-------+ +-------+ +-------+
     |       | |       | |       |
     | app 1 | | app 2 | | app N |      1: Applications
     |       | |       | |       |
   ---------------------------------    2: libprofile.h
            |             |
            | libprofile  |
            |             |             3: API library
            +-------------+
                 ^ ^ ^
                 | | |
                 D-Bus                  4: IPC communication
+--------+       | | |
| static |       v v v                  5: Default values
| config |    +----------+
| files  |--->|          |
+--------+    | profiled |              6:Server Process
              |          |
              +----------+
               ^        ^
               |        |
               |        |
               v        v
       +---------+    +---------+
       | dynamic |    | current |       7: Modified values
       | config  |    | profile |       8: Active profile
       | file    |    | file    |
       +---------+    +---------+
@endcode

@section svn SVN

All source code is stored in subversion repository
@e dms, subdirectory @e profiled.

*/
