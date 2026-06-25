#ifndef INSTRUCTION_OBFUSCATOR_H
#define INSTRUCTION_OBFUSCATOR_H

void instruction_obfuscator_init(void);
void instruction_obfuscator_shutdown(void);
void obfuscated_call(void (*func)(void));
void obfuscated_call_vargs(void (*func)(void*), void *arg);

#endif
