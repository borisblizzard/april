/************************************************************************************\
This source file is part of the Awesome Portable Rendering Interface Library         *
For latest info, see http://libapril.sourceforge.net/                                *
**************************************************************************************
Copyright (c) 2010 Ivan Vucica (ivan@vucica.net)                                     *
*                                                                                    *
* This program is free software; you can redistribute it and/or modify it under      *
* the terms of the BSD license: http://www.opensource.org/licenses/bsd-license.php   *
\************************************************************************************/
#ifndef APRIL_MAIN_H
#define APRIL_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __APPLE__

int april_real_main(int argc, char** argv);
int april_main(int(*real_main)(int argc, char** argv), int argc, char** argv);
int main(int argc, char** argv);

#define main april_real_main

#else

#endif


#ifdef __cplusplus
} // extern C
#endif

	
#endif
