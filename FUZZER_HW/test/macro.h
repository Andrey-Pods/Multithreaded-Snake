#ifndef MACRO_H
#define MACRO_H

#define STR(a) #a

#define PRIMITIVE_CAT(a, b) a ## b
#define CAT(a, b) PRIMITIVE_CAT(a, b)

#endif