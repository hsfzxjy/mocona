#ifndef PORT_H
#define PORT_H

#ifdef _MSC_VER
#define PROTECTED extern
#else
#define PROTECTED __attribute__((visibility("hidden")))
#endif

#endif