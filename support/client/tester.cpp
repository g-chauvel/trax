/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * This is an example of a stationary tracker. It only reports the initial
 * position for all frames and is used for testing purposes.
 * The main function of this example is to show the developers how to modify
 * their trackers to work with the evaluation environment.
 *
 * Copyright (c) 2015, Luka Cehovin
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <cstring>

#include <stdexcept>
#include <iostream>
#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <streambuf>

#include <trax/client.hpp>

#include "getopt.h"

#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
#include <ctype.h>
#include <windows.h>

#define strcmpi _strcmpi

__inline void sleep(long time) {
    Sleep(time * 1000);
}

#define __INLINE __inline

#else

#ifdef _MAC_

#else
    #include <unistd.h>
#endif

#include <signal.h>
#define strcmpi strcasecmp
#define __INLINE inline

#endif

using namespace std;
using namespace trax;

#ifndef TRAX_BUILD_DATE
#define TRAX_BUILD_DATE __DATE__
#endif

void print_help() {

    cout << "TraX Test utility" << "\n\n";
    cout << "Built on " << TRAX_BUILD_DATE << "\n";
    cout << "Library version: " << trax_version() << "\n";
    cout << "Protocol version: " << TRAX_VERSION << "\n\n";

    cout << "Usage: traxtest [-h] [-d] \n";
    cout << "\t [-e name=value] [-p name=value]\n";
    cout << "\t [-t timeout] [-x] [-X]\n";
    cout << "\t -- <command_part1> <command_part2> ...";

    cout << "\n\nProgram arguments: \n";
    cout << "\t-d\tEnable debug\n";
    cout << "\t-e\tEnvironmental variable (multiple occurences allowed)\n";
    cout << "\t-h\tPrint this help and exit\n";
    cout << "\t-p\tTracker parameter (multiple occurences allowed)\n";
    cout << "\t-t\tSet timeout period\n";
    cout << "\t-x\tUse explicit streams, not standard ones.\n";
    cout << "\t-X\tUse TCP/IP sockets instead of file streams.\n";
    cout << "\n";

    cout << "\n";
}

void configure_signals() {

    #if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)

    // TODO: anything to do here?

    #else

    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags =  SA_RESTART | SA_NOCLDSTOP; //SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &sa, 0) == -1) {
      perror(0);
      exit(1);
    }

    #endif

}

#ifndef MAX
#define MAX(a,b) ((a) > (b)) ? (a) : (b)
#endif

string get_temp_name(const string& file) {
#if defined(__OS2__) || defined(__WINDOWS__) || defined(WIN32) || defined(WIN64) || defined(_MSC_VER)
string result;
char chars[MAX_PATH];
if (GetTempPath(MAX_PATH, chars))
    return string(chars) + string("\\") + file;
else
    return string("C:\\Temp");
#else
    if (getenv("TMPDIR"))
        return string(getenv("TMPDIR")) + string("/") + file;
    else
        return string("/tmp/") + file;
#endif
}

#define tester_header_only
#include "tester_resources.c"

#define CMD_OPTIONS "hdt:p:e:xX"

#define DEBUGMSG(...) if (verbosity == VERBOSITY_DEBUG) { fprintf(stdout, "CLIENT: "); fprintf(stdout, __VA_ARGS__); }

int main(int argc, char** argv) {

    int c;
    opterr = 0;
    int result = 0;
    ConnectionMode connection = CONNECTION_DEFAULT;
    VerbosityMode verbosity = VERBOSITY_SILENT;
    int timeout = 30;

    string tracker_command;

    Properties properties;
    map<string, string> environment;

    configure_signals();

    try {

        while ((c = getopt(argc, argv, CMD_OPTIONS)) != -1)
            switch (c) {
            case 'h':
                print_help();
                exit(0);
            case 'd':
                verbosity = VERBOSITY_DEBUG;
                break;
            case 'x':
                connection = CONNECTION_EXPLICIT;
                break;
            case 'X':
                connection = CONNECTION_SOCKETS;
                break;
            case 't':
                timeout = MAX(0, atoi(optarg));
                break;
            case 'e': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                environment[string(var, ptr - var)] = string(ptr + 1);
                break;
            }
            case 'p': {
                char* var = optarg;
                char* ptr = strchr(var, '=');
                if (!ptr) break;
                string key(var, ptr - var);
                string value(ptr + 1);
                properties.set(key, value);
                break;
            }
            default:
                print_help();
                throw std::runtime_error(string("Unknown switch -") + string(1, (char) optopt));
            }

        if (optind < argc) {

            stringstream buffer;

            for (int i = optind; i < argc; i++) {
                buffer << " \"" << string(argv[i]) << "\" ";
            }

            tracker_command = buffer.str();

        } else {
            print_help();
            exit(-1);
        }

        TrackerProcess tracker(tracker_command, environment, timeout, connection, verbosity);

        Image image;

        int image_formats = tracker.metadata().image_formats();

        if TRAX_SUPPORTS(image_formats, TRAX_IMAGE_BUFFER) {
            size_t length;
            char* buffer = NULL;
            tester_copy_resource("static.jpg", &buffer, &length);
            image = Image::create_buffer(length, buffer);
            free(buffer);
        } else if TRAX_SUPPORTS(image_formats, TRAX_IMAGE_MEMORY) {
            size_t length;
            char* buffer = NULL;
            tester_copy_resource("static.bin", &buffer, &length);
            image = Image::create_memory(320, 240, TRAX_IMAGE_MEMORY_RGB);
            char* dst = image.write_memory_row(0);
            memcpy(dst, buffer, 320 * 240);
            free(buffer);
        } else if TRAX_SUPPORTS(image_formats, TRAX_IMAGE_PATH) {
            size_t length;
            char* buffer = NULL;
            tester_copy_resource("static.jpg", &buffer, &length);
            string temp_path = get_temp_name("trax_testing.jpg");
            std::ofstream output(temp_path.c_str(), std::ios::out | std::ios::binary );
            output.write(buffer, length);
            image = Image::create_path(temp_path);
            free(buffer);
        }
        else
            throw std::runtime_error("No supported image format allowed");

        Region initialization_region = Region::create_rectangle(130, 80, 70, 110);

        int frame = 0;
        int initializations = 0;
        while (initializations < 5) {

            if (!tracker.ready()) {
                throw std::runtime_error("Tracker process not alive anymore.");
            }

            if (!tracker.initialize(image, initialization_region, properties)) {
                throw std::runtime_error("Unable to initialize tracker.");
            }

            initializations++;

            bool initialized = true;

            frame = 0;

            while (frame < 20) {
                // Repeat while tracking the target.
                Region status;
                Properties additional;

                bool result = tracker.wait(status, additional);

                if (result) {
                    // Default option, the tracker returns a valid status.

                    Region storage;
                    initialized = false;

                } else {
                    if (tracker.ready()) {
                        // The tracker has requested termination of connection.
                        DEBUGMSG("Termination requested by tracker.\n");
                        break;
                    } else {
                        // In case of an error ...
                        throw std::runtime_error("Unable to contact tracker.");
                    }
                }

                Properties no_properties;
                if (!tracker.frame(image, no_properties))
                    throw std::runtime_error("Unable to send new frame.");

                frame++;

            }

        }

    } catch (std::runtime_error e) {

        fprintf(stderr, "Error: %s\n", e.what());
        result = -1;

    }

    exit(result);

}
