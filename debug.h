#pragma once

#ifdef DEBUG
#define LOGF(...) printf(__VA_ARGS__)
#define LOGS std::cout
#else
#define LOGF(...) 
#define LOGS if(false) std::cout
#endif
