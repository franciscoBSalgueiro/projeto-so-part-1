#ifndef _PRETTY_PRINT_H
#define _PRETTY_PRINT_H

#include <stdio.h>

#define GREEN "\033[32m"

#define RESET "\033[0m"

#define PRINT_GREEN(...) printf(GREEN __VA_ARGS__ RESET)

#endif