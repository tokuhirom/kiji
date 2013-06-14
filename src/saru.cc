#include <stdio.h>
#include <assert.h>
#include "node.h"
#include "gen.saru.y.cc"
extern "C" {
#include "moarvm.h"
}

namespace saru {
    namespace Assembler {
        class MVM {
        public:
            void write(MVMuint8 bank_num, MVMuint8 op_num) {
                write_8(bank_num);
                write_8(op_num);
            }
            void write(MVMuint8 bank_num, MVMuint8 op_num, MVMuint16 op1, MVMuint16 op2) {
                write(bank_num, op_num);
                write_16(op1);
                write_16(op2);
            }
            void write_8(MVMuint8 i) {
                bytecode_.push_back(i);
            }
            void write_16(MVMuint16 i) {
                bytecode_.push_back(i&0xffff);
                bytecode_.push_back(i>>4);
            }
            MVMuint8* bytecode() {
                return bytecode_.data(); // C++11
            }
            size_t bytecode_size() {
                return bytecode_.size();
            }
        private:
            std::vector<MVMuint8> bytecode_;
        };
    }
}


static void toplevel_initial_invoke(MVMThreadContext *tc, void *data) {
    /* Dummy, 0-arg callsite. */
    static MVMCallsite no_arg_callsite;
    no_arg_callsite.arg_flags = NULL;
    no_arg_callsite.arg_count = 0;
    no_arg_callsite.num_pos   = 0;

    /* Create initial frame, which sets up all of the interpreter state also. */
    MVM_frame_invoke(tc, (MVMStaticFrame *)data, &no_arg_callsite, NULL, NULL, NULL);
}

void runit(int argc) { 
    MVMInstance * vm = MVM_vm_create_instance();

    // init compunit.
    MVMThreadContext *tc = vm->main_thread;
    MVMCompUnit      *cu = (MVMCompUnit*)malloc(sizeof(MVMCompUnit));
    memset(cu, 0, sizeof(MVMCompUnit));
    int          apr_return_status;
    apr_pool_t  *pool        = NULL;
    /* Ensure the file exists, and get its size. */
    if ((apr_return_status = apr_pool_create(&pool, NULL)) != APR_SUCCESS) {
        MVM_panic(MVM_exitcode_compunit, "Could not allocate APR memory pool: errorcode %d", apr_return_status);
    }
    cu->pool       = pool;
    cu->num_frames  = 1;
    cu->main_frame = (MVMStaticFrame*)malloc(sizeof(MVMStaticFrame));
    cu->frames = (MVMStaticFrame**)malloc(sizeof(MVMStaticFrame) * 1);
    cu->frames[0] = cu->main_frame;
    memset(cu->main_frame, 0, sizeof(MVMStaticFrame));
    cu->main_frame->cuuid = MVM_string_utf8_decode(tc, tc->instance->VMString, "cuuid", strlen("cuuid"));
    cu->main_frame->name = MVM_string_utf8_decode(tc, tc->instance->VMString, "main frame", strlen("main frame"));
    assert(cu->main_frame->cuuid);
    cu->main_frame->cu = cu;

    saru::Assembler::MVM assembler;
    assembler.write(MVM_OP_BANK_primitives, MVM_OP_const_s, 0, 0);
    assembler.write(MVM_OP_BANK_io,         MVM_OP_say,     0, 0);
    assembler.write(MVM_OP_BANK_primitives, MVM_OP_return);

    cu->main_frame->bytecode      = assembler.bytecode();
    cu->main_frame->bytecode_size = assembler.bytecode_size();

    cu->main_frame->work_size = 0;

    cu->main_frame->num_locals= 1; // register size
    cu->main_frame->local_types = (MVMuint16*)malloc(sizeof(MVMuint16)*cu->main_frame->num_locals);
    cu->main_frame->local_types[0] = MVM_reg_str;

    cu->strings = (MVMString**)malloc(sizeof(MVMString)*1);
    cu->strings[0] = MVM_string_utf8_decode(tc, tc->instance->VMString, "Hello, world!", strlen("Hello, world!"));
    cu->num_strings = 1;

    // And fill bytecode.
    MVMStaticFrame *start_frame = cu->main_frame ? cu->main_frame : cu->frames[0];

    if (argc>1) {
        // dump it
        char *dump = MVM_bytecode_dump(tc, cu);
        
        printf("%s", dump);
        free(dump);
    } else {
        // Or, run it.
        MVM_interp_run(tc, &toplevel_initial_invoke, start_frame);
    }

    // cleanup
    MVM_vm_destroy_instance(vm);
}

int main(int argc, char** argv)
{
    GREG g;
    line_number=0;
    yyinit(&g);
    if (!yyparse(&g)) {
        fprintf(stderr, "** Syntax error at line %d\n", line_number);
        if (g.text[0]) {
            fprintf(stderr, "** near %s\n", g.text);
        }
        if (g.pos < g.limit || !feof(stdin)) {
            g.buf[g.limit]= '\0';
            fprintf(stderr, " before text \"");
            while (g.pos < g.limit) {
                if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
                fputc(g.buf[g.pos++], stderr);
            }
            if (g.pos == g.limit) {
            int c;
            while (EOF != (c= fgetc(stdin)) && '\n' != c && '\r' != c)
                fputc(c, stderr);
            }
            fputc('\"', stderr);
        }
        fprintf(stderr, "\n");
        exit(1);
    }
    yydeinit(&g);
    printf("Done parsing\n");

    runit(argc);
    // nqpc_dump_node(node_global);

    return 0;
}
