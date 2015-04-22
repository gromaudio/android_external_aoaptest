#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUGOUT_F(...) do{fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");}while(0)
#define DEBUGOUT(x)     do{fprintf(stderr, x); fprintf(stderr, "\n");}while(0)


#endif /* __DEBUG_H__ */
