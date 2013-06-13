%{
#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include <string>
#include "node.h"

class NQPCNode {
public:
    NQPCNode() : type_(NQPC_NODE_UNDEF) { }
    NQPCNode(const NQPCNode &node) {
        this->type_ = node.type_;
        switch (type_) {
        case NQPC_NODE_IDENT:
            this->body_.pv = new std::string(*(node.body_.pv));
            break;
        case NQPC_NODE_INT:
            this->body_.iv = node.body_.iv;
            break;
        case NQPC_NODE_NUMBER:
            this->body_.nv = node.body_.nv;
            break;
        case NQPC_NODE_ARGS:
        case NQPC_NODE_FUNCALL:
        case NQPC_NODE_STATEMENTS:
        case NQPC_NODE_MUL:
        case NQPC_NODE_DIV:
        case NQPC_NODE_ADD:
        case NQPC_NODE_SUB:
            this->body_.children = new std::vector<NQPCNode>();
            *(this->body_.children) = *(node.body_.children);
            break;
        case NQPC_NODE_UNDEF:
            abort();
            break;
        }
    }
    ~NQPCNode() {
        switch (type_) {
        case NQPC_NODE_IDENT: {
            break;
        }
        case NQPC_NODE_INT:
            break;
        case NQPC_NODE_NUMBER:
            break;
        case NQPC_NODE_ARGS:
        case NQPC_NODE_FUNCALL:
        case NQPC_NODE_STATEMENTS:
        case NQPC_NODE_MUL:
        case NQPC_NODE_DIV:
        case NQPC_NODE_ADD:
        case NQPC_NODE_SUB:
            // delete this->body_.children;
            break;
        case NQPC_NODE_UNDEF:
            break;
        }
    }

    void set(NQPC_NODE_TYPE type, const NQPCNode &child) {
        this->type_ = type;
        this->body_.children = new std::vector<NQPCNode>();
        this->body_.children->push_back(child);
    }
    void set(NQPC_NODE_TYPE type, const NQPCNode &c1, const NQPCNode &c2) {
        this->type_ = type;
        this->body_.children = new std::vector<NQPCNode>();
        this->body_.children->push_back(c1);
        this->body_.children->push_back(c2);
    }
    void set_children(NQPC_NODE_TYPE type) {
        this->type_ = type;
        this->body_.children = new std::vector<NQPCNode>();
    }
    void set_number(const char*txt) {
        this->type_ = NQPC_NODE_NUMBER;
        this->body_.nv = atof(txt);
    }
    void set_integer(const char*txt, int base) {
        this->type_ = NQPC_NODE_INT;
        this->body_.iv = strtol(txt, NULL, base);
    }
    void set_ident(const char *txt, int length) {
        this->type_ = NQPC_NODE_IDENT;
        this->body_.pv = new std::string(txt, length);
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
        return *(this->body_.children);
    }
    void push_child(NQPCNode &child) {
        assert(this->type_ != NQPC_NODE_INT);
        assert(this->type_ != NQPC_NODE_NUMBER);
        assert(this->type_ != NQPC_NODE_IDENT);
        this->body_.children->push_back(child);
    }
    void negate() {
        if (this->type_ == NQPC_NODE_INT) {
            this->body_.iv = - this->body_.iv;
        } else {
            this->body_.nv = - this->body_.nv;
        }
    }
    NQPC_NODE_TYPE type() const { return type_; }
    const std::string pv() const {
        return *(this->body_.pv);
    }

private:
    NQPC_NODE_TYPE type_;
    union {
        long int iv; // integer value
        double nv; // number value
        std::string *pv;
        std::vector<NQPCNode> *children;
    } body_;
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
        // Node has a PV
    case NQPC_NODE_IDENT:
        indent(depth+1);
        printf("\"value\":\"%s\"\n", node.pv().c_str()); // TODO need escape
        break;
        // Node has children
    case NQPC_NODE_ARGS:
    case NQPC_NODE_FUNCALL:
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

static void nqpc_dump_node(const NQPCNode &node) {
    nqpc_dump_node(node, 0);
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
        s1.push_child(s2);
        $$ = s1;
    } )* eat_terminator?

# TODO
statement = e:expr ws* { $$ = e; }

args =
    (
        s1:expr {
            $$.set(NQPC_NODE_ARGS, s1);
            s1 = $$;
        }
        ( ',' s2:expr {
            s1.push_child(s2);
            $$ = s1;
        } )*
    )
    | '' { $$.set_children(NQPC_NODE_ARGS); }

expr = funcall_expr

funcall_expr =
    i:ident '(' a:args ')' {
        $$.set(NQPC_NODE_FUNCALL, i, a);
    }
    | add_expr

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

ident = < [a-zA-Z] [a-zA-Z0-9]+ ( [_-] [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
}

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

    nqpc_dump_node(node_global);

    return 0;
}
