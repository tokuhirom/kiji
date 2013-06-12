%{
#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "node.h"

class NQPCNode {
public:
    NQPCNode() : type_(NQPC_NODE_UNDEF) { }
    NQPCNode(const NQPCNode &node) {
        this->type_ = node.type_;
        switch (type_) {
        case NQPC_NODE_INT:
            this->body_.iv = node.body_.iv;
            break;
        case NQPC_NODE_NUMBER:
            this->body_.nv = node.body_.nv;
            break;
        case NQPC_NODE_STATEMENTS:
        case NQPC_NODE_MUL:
        case NQPC_NODE_DIV:
        case NQPC_NODE_ADD:
        case NQPC_NODE_SUB:
            this->children_ = node.children_;
            break;
        case NQPC_NODE_UNDEF:
            abort();
        }
    }

    void set(NQPC_NODE_TYPE type, const NQPCNode &child) {
        this->type_ = type;
        this->children_.clear();
        this->children_.push_back(child);
    }
    void set(NQPC_NODE_TYPE type, const NQPCNode &c1, const NQPCNode &c2) {
        this->type_ = type;
        this->children_.clear();
        this->children_.push_back(c1);
        this->children_.push_back(c2);
    }
    void set_number(const char*txt) {
        this->type_ = NQPC_NODE_NUMBER;
        this->body_.nv = atof(txt);
    }
    void set_integer(const char*txt, int base) {
        this->type_ = NQPC_NODE_INT;
        this->body_.iv = strtol(txt, NULL, base);
    }

    long int iv() const {
        assert(this->type_ == NQPC_NODE_INT);
        return this->body_.iv;
    } 
    double nv() const {
        assert(this->type_ == NQPC_NODE_NUMBER);
        return this->body_.nv;
    } 
    const std::vector<NQPCNode> & children() const {
        return children_;
    }
    void push_children(NQPCNode &child) {
        this->children_.push_back(child);
    }
    void negate() {
        if (this->type_ == NQPC_NODE_INT) {
            this->body_.iv = - this->body_.iv;
        } else {
            this->body_.nv = - this->body_.nv;
        }
    }
    NQPC_NODE_TYPE type() const { return type_; }

private:
    NQPC_NODE_TYPE type_;
    union {
        long int iv; // integer value
        double nv; // number value
    } body_;
    std::vector<NQPCNode> children_;
};

static NQPCNode node_global;
static int line_number;

#define YYSTYPE NQPCNode

static void indent(int n) {
    for (int i=0; i<n*4; i++) {
        printf(" ");
    }
}

static void nqpc_dump_node(const NQPCNode &node, unsigned int depth) {
    printf("{\n");
    indent(depth+1);
    printf("\"type\":\"%s\",\n", nqpc_node_type2name(node.type()));
    switch (node.type()) {
    case NQPC_NODE_INT:
        indent(depth+1);
        printf("\"value\":%ld\n", node.iv());
        break;
    case NQPC_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", node.nv());
        break;
        // Statement has children
    case NQPC_NODE_MUL:
    case NQPC_NODE_ADD:
    case NQPC_NODE_SUB:
    case NQPC_NODE_DIV:
    case NQPC_NODE_STATEMENTS: {
        indent(depth+1);
        printf("\"value\":[\n");
        int i=0;
        for (auto &x:node.children()) {
            indent(depth+2);
            nqpc_dump_node(
                x, depth+2
            );
            if (i==node.children().size()-1) {
                printf("\n");
            } else {
                printf(",\n");
            }
            i++;
        }
        indent(depth+1);
        printf("]\n");
        break;
    }
    case NQPC_NODE_UNDEF:
        break;
    }
    indent(depth);
    printf("}");
    if (depth == 0) {
        printf("\n");
    }
}

%}

comp_init = e:statementlist end-of-file {
    $$ = (node_global = e);
}

statementlist =
    s1:statement {
        $$.set(NQPC_NODE_STATEMENTS, s1);
        s1 = $$;
    }
    ( eat_terminator s2:statement {
        s1.push_children(s2);
        $$ = s1;
    } )* eat_terminator?

# TODO
statement = e:expr ws* { $$ = e; }

expr = add_expr

add_expr =
    l:mul_expr (
          '+' r1:mul_expr {
            $$.set(NQPC_NODE_ADD, l, r1);
            l = $$;
          }
        | '-' r2:mul_expr {
            $$.set(NQPC_NODE_SUB, l, r2);
            l = $$;
          }
    )* {
        $$ = l;
    }

mul_expr =
    l:term (
        '*' r:term {
            $$.set(NQPC_NODE_MUL, l, r);
            l = $$;
        }
        | '/' r:term {
            $$.set(NQPC_NODE_DIV, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

term = value

value = 
    ( '-' ( integer | dec_number) ) {
        $$.negate();
    }
    | integer
    | dec_number

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    ';' | end-of-file

dec_number =
    <([.][0-9]+)> {
    $$.set_number(yytext);
}
    | <([0-9]+ '.' [0-9]+)> {
    $$.set_number(yytext);
}
    | <([0-9]+)> {
    $$.set_integer(yytext, 10);
}

integer =
    '0b' <[01]+> {
    $$.set_integer(yytext, 2);
}
    | '0x' <[0-9a-f]+> {
    $$.set_integer(yytext, 16);
}
    | '0o' <[0-7]+> {
    $$.set_integer(yytext, 8);
}

# TODO

# white space
ws = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
    | '#' [^\n]*
end-of-line = ( '\r\n' | '\n' | '\r' ) {
    line_number++;
}
end-of-file = !'\0'

%%

int main()
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

    nqpc_dump_node(node_global, 0);

    return 0;
}
