/**************************************************************/
/* relsort.cpp                                                */
/* Copyleft 2005 by Max Alekseyev                            */
/**************************************************************/
/*
 *   It is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *
 * Revision History:
 *
 *    Sten        22/07/2006
 *        Code ported and tested on Win32 platform.
 *        Several bugs fixed.
 *        Minor style improvements.
 *
 *    Max         Some time ago..
 *        Initial release
 *
 */

#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE
#define _LARGEFILE64_SOURCE

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <iostream>
#include <iomanip>
#include <sstream>

#include <glob.h>
#include <map>
using namespace std;

#if (defined(_WIN32) || defined(_WIN64))
  #define CAT "type"
#else
  #define CAT "cat"
#endif

const size_t limit = 1ul << 31;   // 2GB limit per resulting container

typedef multimap<size_t,string> fmap_t;

int main(int argc, char* argv[]) 
{

    if (argc != 3) 
    {
	cout << "Usage: " << argv[0] << " <ibase> <obase>" << endl;
        return 1;
    }

    string ibase((string)argv[1] + "*");
    string obase(argv[2]);

    size_t totalsize = 0;

    fmap_t M;

    glob_t glb;
    glob(ibase.c_str(), GLOB_NOSORT, 0, &glb);

//    clog << "Quering size ";
    for(int i=0;i<glb.gl_pathc;++i) 
    {
//	cout << glb.gl_pathv[i] << endl;
//        clog << "."; clog.flush();

        struct stat st;
        if (stat(glb.gl_pathv[i], &st) != 0)
        {
            cout << "Error: unable to obtain filesize information for '" << glb.gl_pathv[i] << "'." << endl;
            return 1;
        }

	M.insert(make_pair(st.st_size, string(glb.gl_pathv[i])));
        totalsize += st.st_size;
    }

//    clog << endl << "Files: " << glb.gl_pathc << "\tTotal size: " << totalsize << endl;

    int blk = 0;

    while (!M.empty()) 
    {
	size_t sz = limit;
//	clog << "Block " << blk << " : ";
        cout << CAT;

        //
        // Sten: I replaced reverse_iterator with iterator since
        //       M.erase(im.base()) construct didn't work on my MinGW build.
        //       I'm to lazy to dig why it is so.
        //
search_again:
	for(fmap_t::iterator im = M.begin(); im != M.end(); im++) 
        {
	    if (im->first <= sz) 
            {
		sz -= im->first;
		cout << " " << im->second.c_str();
      	        M.erase(im);       // Sten: Map has been modified, so we have no
                goto search_again; //       guaranee our iterators are valid any longer.
                                   //       It's safer to start search again.
	    }           
	}

//	clog << "size " << limit - sz << endl;
//      clog.flush();

	ostringstream of;
        of << obase << "." << setfill('0') << setw(3) << blk;
	cout << " > " << of.str() << endl;
        blk++;
    }

    globfree(&glb);
    return 0;
}
