FLAGS:=										\
-I include									\
-g 											\
-msse4.2 									\
-mavx2										\
-no-pie 									\
-D _DEBUG									\
-ggdb3										\
-std=c++17									\
-O2											\
-Wall										\
-Wextra										\
-Weffc++									\
-Waggressive-loop-optimizations				\
-Wc++14-compat								\
-Wmissing-declarations						\
-Wcast-align								\
-Wcast-qual									\
-Wchar-subscripts							\
-Wconditionally-supported					\
-Wconversion								\
-Wctor-dtor-privacy							\
-Wempty-body								\
-Wfloat-equal								\
-Wformat-nonliteral							\
-Wformat-security							\
-Wformat-signedness							\
-Wformat=2									\
-Winline									\
-Wlogical-op								\
-Wnon-virtual-dtor							\
-Wopenmp-simd								\
-Woverloaded-virtual						\
-Wpacked									\
-Wpointer-arith								\
-Winit-self									\
-Wredundant-decls							\
-Wshadow									\
-Wsign-conversion							\
-Wsign-promo								\
-Wstrict-null-sentinel						\
-Wstrict-overflow=2							\
-Wsuggest-attribute=noreturn				\
-Wsuggest-final-methods						\
-Wsuggest-final-types						\
-Wsuggest-override							\
-Wswitch-default							\
-Wswitch-enum								\
-Wsync-nand -Wundef							\
-Wunreachable-code							\
-Wunused									\
-Wuseless-cast								\
-Wvariadic-macros							\
-Wno-literal-suffix							\
-Wno-missing-field-initializers				\
-Wno-narrowing								\
-Wno-old-style-cast							\
-Wno-varargs								\
-Wstack-protector							\
-fcheck-new									\
-fsized-deallocation						\
-fstack-protector							\
-fstrict-overflow							\
-flto-odr-type-merging						\
-fno-omit-frame-pointer						\
-Wlarger-than=8192							\
-Wstack-usage=8192							\
-Werror=vla									\
-static-libasan 							\
-fno-inline-functions						\
-fno-inline-small-functions					\

BINDIR:=bin
OUTPUT:=hashtab
SRCDIR:=source
SOURCE:=$(wildcard ${SRCDIR}/*.cpp)
OBJECTS:=$(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SOURCE}))))
ASM_OBJ:=${BINDIR}/cmp_key.o
ASM_SRC:=${SRCDIR}/cmp_key.asm

all: ${OUTPUT}

${OUTPUT}:${OBJECTS} ${ASM_OBJ}
	g++ ${FLAGS} ${ASM_OBJ} ${OBJECTS} -o ${BINDIR}/${OUTPUT}
${ASM_OBJ}: ${ASM_SRC}
	nasm -felf64 ${ASM_SRC} -o ${ASM_OBJ}
${ASM_SRC}:

${OBJECTS}: ${SOURCE} ${BINDIR}
	$(foreach SRC,${SOURCE},$(shell g++ -c ${SRC} ${FLAGS} -o $(addsuffix .o,$(addprefix ${BINDIR}/,$(basename $(notdir ${SRC}))))))
clean:
	rm -rf ${BINDIR}
${SOURCE}:

${BINDIR}:
	mkdir ${BINDIR}
