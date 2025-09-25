#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define LKJSCRIPT_PATH "/data/main.lkjscript"
#define MAX_STACK_SIZE 4096
#define MAX_VARS 256
#define MAX_PARAMS 16
#define MAX_MEMBERS 32

// Forward declarations
typedef enum type_t type_t;
typedef struct token_t token_t;
typedef struct node_t node_t;
typedef struct val_t val_t;
typedef struct env_t env_t;
typedef struct var_info_t var_info_t;
typedef struct fn_info_t fn_info_t;
typedef struct struct_info_t struct_info_t;
typedef struct member_info_t member_info_t;
typedef struct stack_frame_t stack_frame_t;
typedef struct eval_ctx_t eval_ctx_t;

// Forward declarations for parser
static node_t* parse_stmt(token_t** tokens);
static node_t* parse_expr(token_t** tokens);
static node_t* parse_block(token_t** tokens);
static node_t* parse_var_decl(token_t** tokens);
static node_t* parse_fn_decl(token_t** tokens);
static node_t* parse_struct_decl(token_t** tokens);

// Forward declarations for evaluator
static val_t eval(eval_ctx_t* ctx, node_t* node);
static val_t value_null(void);
static val_t value_i64(int64_t value);
static void ensure_i64(val_t value, const char* context);
static size_t eval_collect_args(eval_ctx_t* ctx, node_t* node, val_t* args, size_t max_args);

enum type_t {
    TYPE_NULL,
    TYPE_BLOCK,
    TYPE_IDENT,
    TYPE_I64,
    TYPE_STR,
    TYPE_BOOL,
    TYPE_PTR,
    
    // Declarations
    TYPE_VAR,           // var x = 5
    TYPE_FN,            // fn name(params) { body }
    TYPE_STRUCT,        // struct Name { members }
    TYPE_RETURN,        // return expr
    
    // Expressions
    TYPE_CALL,          // function call
    TYPE_MEMBER,        // struct.member
    TYPE_INDEX,         // array[index]
    
    // Binary operations
    TYPE_ADD, TYPE_SUB, TYPE_MUL, TYPE_DIV, TYPE_MOD,
    TYPE_EQ, TYPE_NEQ, TYPE_LT, TYPE_GT, TYPE_LTE, TYPE_GTE,
    TYPE_AND, TYPE_OR, TYPE_NOT,
    TYPE_ASSIGN,
    
    // Control flow
    TYPE_IF, TYPE_ELSE, TYPE_WHILE, TYPE_FOR,
    TYPE_BREAK, TYPE_CONTINUE,
    
    // System calls
    TYPE_SYSCALL_DEBUG01,
    TYPE_SYSCALL_WRITE,
    TYPE_SYSCALL_READ,
    TYPE_SYSCALL_OPEN,
    TYPE_SYSCALL_CLOSE,
    TYPE_SYSCALL_MALLOC,
    TYPE_SYSCALL_FREE,
};

struct token_t {
    const char* data;
    uint64_t size;
    token_t* next;
};

struct member_info_t {
    token_t* name;
    type_t type;
    size_t offset;
    size_t size;
};

struct struct_info_t {
    token_t* name;
    member_info_t members[MAX_MEMBERS];
    size_t member_count;
    size_t total_size;
};

struct var_info_t {
    token_t* name;
    type_t type;
    int64_t stack_offset;  // negative for locals, positive for params
    bool is_param;
    bool is_global;
    struct_info_t* struct_type;
};

struct fn_info_t {
    token_t* name;
    var_info_t params[MAX_PARAMS];
    size_t param_count;
    var_info_t locals[MAX_VARS];
    size_t local_count;
    size_t stack_size;
    node_t* body;
};

struct node_t {
    type_t type;
    node_t* next;
    node_t* parent;
    node_t* child_begin;
    node_t* child_end;
    int64_t value_i64;
    char* value_str;
    token_t* value_token;
    
    // Function/variable metadata
    fn_info_t* fn_info;
    var_info_t* var_info;
    struct_info_t* struct_info;
    
    // Stack offset for local variables
    int64_t stack_offset;
};

struct val_t {
    type_t type;
    union {
        int64_t i64;
        char* str;
        void* ptr;
        bool b;
    } value;
};

struct stack_frame_t {
    char data[MAX_STACK_SIZE];
    size_t size;
    fn_info_t* fn_info;
    stack_frame_t* prev;
};

struct env_t {
    fn_info_t functions[MAX_VARS];
    size_t fn_count;
    
    var_info_t globals[MAX_VARS];
    size_t global_count;
    
    struct_info_t structs[MAX_VARS];
    size_t struct_count;
    
    env_t* parent;
};

struct eval_ctx_t {
    env_t* env;
    stack_frame_t* stack;
    char* heap_data[MAX_VARS];
    size_t heap_count;
};

// Global environment
static env_t g_global_env = {0};

// Current function being parsed (for local variable management)
static fn_info_t* g_current_fn = NULL;

static val_t value_null(void) {
    return (val_t){.type = TYPE_NULL, .value.i64 = 0};
}

static val_t value_i64(int64_t value) {
    return (val_t){.type = TYPE_I64, .value.i64 = value};
}

static val_t value_bool(bool b) {
    return (val_t){.type = TYPE_BOOL, .value.b = b};
}

static val_t value_str(char* s) {
    return (val_t){.type = TYPE_STR, .value.str = s};
}

static val_t value_ptr(void* p) {
    return (val_t){.type = TYPE_PTR, .value.ptr = p};
}

static void ensure_i64(val_t value, const char* context) {
    if (value.type != TYPE_I64) {
        fprintf(stderr, "%s: expected i64 but got type %d\n", context, value.type);
        exit(1);
    }
}

static size_t eval_collect_args(eval_ctx_t* ctx, node_t* node, val_t* args, size_t max_args) {
    size_t count = 0;
    for (node_t* child = node->child_begin; child != NULL; child = child->next) {
        if (count >= max_args) {
            fprintf(stderr, "too many arguments (limit %zu)\n", max_args);
            exit(1);
        }
        args[count++] = eval(ctx, child);
    }
    return count;
}

// Token utilities (moved up for use by environment functions)
static bool token_equal(token_t* a, token_t* b) {
    if (!a || !b) return false;
    if (a->size != b->size) return false;
    return strncmp(a->data, b->data, a->size) == 0;
}

static bool token_equal_str(token_t* a, const char* b) {
    if (!a || !b) return false;
    size_t b_size = strlen(b);
    if (a->size != b_size) return false;
    return strncmp(a->data, b, a->size) == 0;
}

// Environment management
static fn_info_t* env_find_function(env_t* env, token_t* name) {
    for (env_t* e = env; e; e = e->parent) {
        for (size_t i = 0; i < e->fn_count; i++) {
            if (token_equal(e->functions[i].name, name)) {
                return &e->functions[i];
            }
        }
    }
    return NULL;
}

static var_info_t* env_find_var(env_t* env, token_t* name) {
    // Check current function's locals first
    if (g_current_fn) {
        for (size_t i = 0; i < g_current_fn->local_count; i++) {
            if (token_equal(g_current_fn->locals[i].name, name)) {
                return &g_current_fn->locals[i];
            }
        }
        for (size_t i = 0; i < g_current_fn->param_count; i++) {
            if (token_equal(g_current_fn->params[i].name, name)) {
                return &g_current_fn->params[i];
            }
        }
    }
    
    // Check global variables
    for (env_t* e = env; e; e = e->parent) {
        for (size_t i = 0; i < e->global_count; i++) {
            if (token_equal(e->globals[i].name, name)) {
                return &e->globals[i];
            }
        }
    }
    return NULL;
}

static struct_info_t* env_find_struct(env_t* env, token_t* name) {
    for (env_t* e = env; e; e = e->parent) {
        for (size_t i = 0; i < e->struct_count; i++) {
            if (token_equal(e->structs[i].name, name)) {
                return &e->structs[i];
            }
        }
    }
    return NULL;
}

// Prevent unused function warnings by having a dummy function that references them
static void __attribute__((unused)) prevent_unused_warnings(void) {
    (void)value_ptr;
    (void)ensure_i64;
    (void)eval_collect_args;
    (void)env_find_struct;
}

// Stack management
static stack_frame_t* stack_push(eval_ctx_t* ctx, fn_info_t* fn_info) {
    stack_frame_t* frame = (stack_frame_t*)calloc(1, sizeof(stack_frame_t));
    frame->fn_info = fn_info;
    // Allocate space for both parameters and locals
    frame->size = (fn_info->param_count + fn_info->local_count) * 8;
    if (frame->size > MAX_STACK_SIZE) frame->size = MAX_STACK_SIZE;
    frame->prev = ctx->stack;
    ctx->stack = frame;
    return frame;
}

static void stack_pop(eval_ctx_t* ctx) {
    if (!ctx->stack) return;
    stack_frame_t* prev = ctx->stack->prev;
    free(ctx->stack);
    ctx->stack = prev;
}

static void* stack_get_var_ptr(eval_ctx_t* ctx, int64_t offset) {
    if (!ctx->stack) return NULL;
    // All variables (parameters and locals) use positive offsets
    if (offset < 0 || (size_t)offset >= ctx->stack->size) return NULL;
    return &ctx->stack->data[offset];
}


int64_t token_to_i64(token_t* a) {
    int64_t value = 0;
    for (uint64_t i = 0; i < a->size; i++) {
        value = value * 10 + (a->data[i] - '0');
    }
    return value;
}

char* file_read(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(length + 2);
    if (!buffer) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    fread(buffer, 1, length, file);
    buffer[length] = '\n';
    buffer[length + 1] = '\0';
    fclose(file);
    return buffer;
}

void tokenize_new(token_t** itr, const char* data, uint64_t size) {
    token_t* new_tok = (token_t*)malloc(sizeof(token_t));
    new_tok->data = data;
    new_tok->size = size;
    new_tok->next = NULL;
    (*itr)->next = new_tok;
    *itr = new_tok;
}

token_t* tokenize(const char* src) {
    token_t head;
    token_t* token_itr = &head;
    const char* src_itr = src;
    
    const char* sign_table[] = {
        "==", "!=", "<=", ">=", "&&", "||", "->", "++", "--",
        "=", "+", "-", "*", "/", "%", "<", ">", "!",
        "(", ")", "{", "}", "[", "]", ",", ";", ".", ":",
        NULL};
    while (*src_itr != '\0') {
        if (*src_itr == '\n') {
            tokenize_new(&token_itr, src_itr, 1);
            src_itr++;
            continue;
        }
        if (*src_itr == ' ' || *src_itr == '\t') {
            src_itr++;
            continue;
        }
        if (*src_itr == '/' && *(src_itr + 1) == '/') {
            while (*src_itr != '\n') {
                src_itr++;
            }
            continue;
        }
        if (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr)) {
            const char* start = src_itr;
            while (strchr("abcdefghijklmnopqrstuvwxyz0123456789_", *src_itr)) {
                src_itr++;
            }
            tokenize_new(&token_itr, start, src_itr - start);
            continue;
        }
        bool sign_found = false;
        for (const char** sign_itr = sign_table; *sign_itr != NULL; sign_itr++) {
            size_t sign_len = strlen(*sign_itr);
            if (strncmp(src_itr, *sign_itr, sign_len) == 0) {
                tokenize_new(&token_itr, src_itr, sign_len);
                src_itr += sign_len;
                sign_found = true;
                break;
            }
        }
        if (!sign_found) {
            fprintf(stderr, "Unknown token starting with: %c\n", *src_itr);
            exit(1);
        }
    }
    return head.next;
}

// Parser utilities
static node_t* node_new(type_t type) {
    node_t* node = (node_t*)calloc(1, sizeof(node_t));
    node->type = type;
    return node;
}

static void node_add_child(node_t* parent, node_t* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    if (!parent->child_begin) {
        parent->child_begin = child;
        parent->child_end = child;
    } else {
        parent->child_end->next = child;
        parent->child_end = child;
    }
}

// Skip tokens
static void skip_newlines(token_t** tokens) {
    while (*tokens && token_equal_str(*tokens, "\n")) {
        *tokens = (*tokens)->next;
    }
}

static bool expect_token(token_t** tokens, const char* expected) {
    skip_newlines(tokens);
    if (!*tokens || !token_equal_str(*tokens, expected)) {
        fprintf(stderr, "Expected '%s' but got '%.*s'\n", 
                expected, (int)(*tokens ? (*tokens)->size : 0), 
                *tokens ? (*tokens)->data : "EOF");
        return false;
    }
    *tokens = (*tokens)->next;
    return true;
}

// Parse variable declaration: var name = expr
static node_t* parse_var_decl(token_t** tokens) {
    *tokens = (*tokens)->next; // skip 'var'
    
    if (!*tokens) {
        fprintf(stderr, "Expected variable name\n");
        return NULL;
    }
    
    node_t* var_node = node_new(TYPE_VAR);
    var_node->value_token = *tokens;
    *tokens = (*tokens)->next;
    
    // Add to local variables if in function
    if (g_current_fn) {
        if (g_current_fn->local_count >= MAX_VARS) {
            fprintf(stderr, "Too many local variables\n");
            return NULL;
        }
        
        var_info_t* var_info = &g_current_fn->locals[g_current_fn->local_count++];
        var_info->name = var_node->value_token;
        var_info->type = TYPE_I64; // Default type
        // Locals stored after parameters: param_count * 8 + local_index * 8
        var_info->stack_offset = (g_current_fn->param_count + g_current_fn->local_count - 1) * 8;
        var_info->is_param = false;
        var_info->is_global = false;
        
        var_node->var_info = var_info;
        var_node->stack_offset = var_info->stack_offset;
        
        g_current_fn->stack_size = (g_current_fn->param_count + g_current_fn->local_count) * 8;
    } else {
        // Global variable
        if (g_global_env.global_count >= MAX_VARS) {
            fprintf(stderr, "Too many global variables\n");
            return NULL;
        }
        
        var_info_t* var_info = &g_global_env.globals[g_global_env.global_count++];
        var_info->name = var_node->value_token;
        var_info->type = TYPE_I64;
        var_info->is_global = true;
        
        var_node->var_info = var_info;
    }
    
    skip_newlines(tokens);
    if (*tokens && token_equal_str(*tokens, "=")) {
        *tokens = (*tokens)->next;
        node_t* init_expr = parse_expr(tokens);
        if (init_expr) {
            node_add_child(var_node, init_expr);
        }
    }
    
    return var_node;
}

// Parse function declaration: fn name(params) { body }
static node_t* parse_fn_decl(token_t** tokens) {
    *tokens = (*tokens)->next; // skip 'fn'
    
    if (!*tokens) {
        fprintf(stderr, "Expected function name\n");
        return NULL;
    }
    
    node_t* fn_node = node_new(TYPE_FN);
    fn_node->value_token = *tokens;
    *tokens = (*tokens)->next;
    
    // Create function info
    if (g_global_env.fn_count >= MAX_VARS) {
        fprintf(stderr, "Too many functions\n");
        return NULL;
    }
    
    fn_info_t* fn_info = &g_global_env.functions[g_global_env.fn_count++];
    fn_info->name = fn_node->value_token;
    fn_node->fn_info = fn_info;
    
    // Set as current function for local variable parsing
    fn_info_t* prev_fn = g_current_fn;
    g_current_fn = fn_info;
    
    // Parse parameters
    if (!expect_token(tokens, "(")) return NULL;
    
    skip_newlines(tokens);
    while (*tokens && !token_equal_str(*tokens, ")")) {
        if (fn_info->param_count >= MAX_PARAMS) {
            fprintf(stderr, "Too many parameters\n");
            return NULL;
        }
        
        var_info_t* param = &fn_info->params[fn_info->param_count];
        param->name = *tokens;
        param->type = TYPE_I64;
        param->stack_offset = fn_info->param_count * 8; // Parameters at positive offsets
        param->is_param = true;
        param->is_global = false;
        
        fn_info->param_count++;
        *tokens = (*tokens)->next;
        
        skip_newlines(tokens);
        if (*tokens && token_equal_str(*tokens, ",")) {
            *tokens = (*tokens)->next;
            skip_newlines(tokens);
        }
    }
    
    if (!expect_token(tokens, ")")) return NULL;
    
    // Parse body
    node_t* body = parse_block(tokens);
    if (body) {
        fn_info->body = body;
        node_add_child(fn_node, body);
    }
    
    // Restore previous function context
    g_current_fn = prev_fn;
    
    return fn_node;
}

// Parse struct declaration: struct Name { members }
static node_t* parse_struct_decl(token_t** tokens) {
    *tokens = (*tokens)->next; // skip 'struct'
    
    if (!*tokens) {
        fprintf(stderr, "Expected struct name\n");
        return NULL;
    }
    
    node_t* struct_node = node_new(TYPE_STRUCT);
    struct_node->value_token = *tokens;
    *tokens = (*tokens)->next;
    
    // Create struct info
    if (g_global_env.struct_count >= MAX_VARS) {
        fprintf(stderr, "Too many structs\n");
        return NULL;
    }
    
    struct_info_t* struct_info = &g_global_env.structs[g_global_env.struct_count++];
    struct_info->name = struct_node->value_token;
    struct_node->struct_info = struct_info;
    
    if (!expect_token(tokens, "{")) return NULL;
    
    size_t offset = 0;
    skip_newlines(tokens);
    while (*tokens && !token_equal_str(*tokens, "}")) {
        if (struct_info->member_count >= MAX_MEMBERS) {
            fprintf(stderr, "Too many struct members\n");
            return NULL;
        }
        
        member_info_t* member = &struct_info->members[struct_info->member_count++];
        member->name = *tokens;
        member->type = TYPE_I64; // Default type
        member->offset = offset;
        member->size = 8; // Default size
        offset += member->size;
        
        *tokens = (*tokens)->next;
        
        skip_newlines(tokens);
        if (*tokens && token_equal_str(*tokens, ";")) {
            *tokens = (*tokens)->next;
            skip_newlines(tokens);
        }
    }
    
    struct_info->total_size = offset;
    
    if (!expect_token(tokens, "}")) return NULL;
    
    return struct_node;
}

// Parse primary expression
static node_t* parse_primary(token_t** tokens) {
    skip_newlines(tokens);
    if (!*tokens) return NULL;
    
    // Numbers
    if ((*tokens)->data[0] >= '0' && (*tokens)->data[0] <= '9') {
        node_t* num_node = node_new(TYPE_I64);
        num_node->value_i64 = token_to_i64(*tokens);
        *tokens = (*tokens)->next;
        return num_node;
    }
    
    // Parenthesized expression
    if (token_equal_str(*tokens, "(")) {
        *tokens = (*tokens)->next;
        node_t* expr = parse_expr(tokens);
        expect_token(tokens, ")");
        return expr;
    }
    
    // System calls
    if (token_equal_str(*tokens, "debug01")) {
        node_t* call_node = node_new(TYPE_SYSCALL_DEBUG01);
        *tokens = (*tokens)->next;
        expect_token(tokens, "(");
        node_t* arg = parse_expr(tokens);
        if (arg) node_add_child(call_node, arg);
        expect_token(tokens, ")");
        return call_node;
    }
    
    // Identifiers and function calls
    if (((*tokens)->data[0] >= 'a' && (*tokens)->data[0] <= 'z') ||
        ((*tokens)->data[0] >= 'A' && (*tokens)->data[0] <= 'Z') ||
        (*tokens)->data[0] == '_') {
        
        node_t* ident_node = node_new(TYPE_IDENT);
        ident_node->value_token = *tokens;
        *tokens = (*tokens)->next;
        
        // Check for function call
        skip_newlines(tokens);
        if (*tokens && token_equal_str(*tokens, "(")) {
            node_t* call_node = node_new(TYPE_CALL);
            node_add_child(call_node, ident_node);
            *tokens = (*tokens)->next;
            
            skip_newlines(tokens);
            while (*tokens && !token_equal_str(*tokens, ")")) {
                node_t* arg = parse_expr(tokens);
                if (arg) node_add_child(call_node, arg);
                
                skip_newlines(tokens);
                if (*tokens && token_equal_str(*tokens, ",")) {
                    *tokens = (*tokens)->next;
                    skip_newlines(tokens);
                }
            }
            
            expect_token(tokens, ")");
            return call_node;
        }
        
        return ident_node;
    }
    
    return NULL;
}

// Parse expression with precedence
static node_t* parse_expr(token_t** tokens) {
    node_t* left = parse_primary(tokens);
    if (!left) return NULL;
    
    skip_newlines(tokens);
    if (!*tokens) return left;
    
    // Handle binary operators (simplified for now)
    if (token_equal_str(*tokens, "+") || token_equal_str(*tokens, "-") ||
        token_equal_str(*tokens, "*") || token_equal_str(*tokens, "/") ||
        token_equal_str(*tokens, "==") || token_equal_str(*tokens, "!=") ||
        token_equal_str(*tokens, "<") || token_equal_str(*tokens, ">") ||
        token_equal_str(*tokens, "=")) {
        
        type_t op_type = TYPE_NULL;
        if (token_equal_str(*tokens, "+")) op_type = TYPE_ADD;
        else if (token_equal_str(*tokens, "-")) op_type = TYPE_SUB;
        else if (token_equal_str(*tokens, "*")) op_type = TYPE_MUL;
        else if (token_equal_str(*tokens, "/")) op_type = TYPE_DIV;
        else if (token_equal_str(*tokens, "==")) op_type = TYPE_EQ;
        else if (token_equal_str(*tokens, "!=")) op_type = TYPE_NEQ;
        else if (token_equal_str(*tokens, "<")) op_type = TYPE_LT;
        else if (token_equal_str(*tokens, ">")) op_type = TYPE_GT;
        else if (token_equal_str(*tokens, "=")) op_type = TYPE_ASSIGN;
        
        *tokens = (*tokens)->next;
        node_t* right = parse_expr(tokens);
        
        if (right) {
            node_t* op_node = node_new(op_type);
            node_add_child(op_node, left);
            node_add_child(op_node, right);
            return op_node;
        }
    }
    
    return left;
}

// Parse block: { statements }
static node_t* parse_block(token_t** tokens) {
    if (!expect_token(tokens, "{")) return NULL;
    
    node_t* block_node = node_new(TYPE_BLOCK);
    
    skip_newlines(tokens);
    while (*tokens && !token_equal_str(*tokens, "}")) {
        node_t* stmt = parse_stmt(tokens);
        if (stmt) {
            node_add_child(block_node, stmt);
        }
        skip_newlines(tokens);
    }
    
    expect_token(tokens, "}");
    return block_node;
}

// Parse statement
static node_t* parse_stmt(token_t** tokens) {
    skip_newlines(tokens);
    if (!*tokens) return NULL;
    
    // Variable declaration
    if (token_equal_str(*tokens, "var")) {
        return parse_var_decl(tokens);
    }
    
    // Function declaration
    if (token_equal_str(*tokens, "fn")) {
        return parse_fn_decl(tokens);
    }
    
    // Struct declaration
    if (token_equal_str(*tokens, "struct")) {
        return parse_struct_decl(tokens);
    }
    
    // Return statement
    if (token_equal_str(*tokens, "return")) {
        *tokens = (*tokens)->next;
        node_t* ret_node = node_new(TYPE_RETURN);
        skip_newlines(tokens);
        if (*tokens && !token_equal_str(*tokens, ";") && !token_equal_str(*tokens, "\n")) {
            node_t* expr = parse_expr(tokens);
            if (expr) node_add_child(ret_node, expr);
        }
        return ret_node;
    }
    
    // Expression statement
    node_t* expr = parse_expr(tokens);
    skip_newlines(tokens);
    if (*tokens && token_equal_str(*tokens, ";")) {
        *tokens = (*tokens)->next;
    }
    return expr;
}

// Parse program
static node_t* parse_program(token_t* tokens) {
    node_t* program = node_new(TYPE_BLOCK);
    token_t* current = tokens;
    
    while (current) {
        node_t* stmt = parse_stmt(&current);
        if (stmt) {
            node_add_child(program, stmt);
        }
    }
    
    return program;
}

// Evaluator
static val_t eval_block(eval_ctx_t* ctx, node_t* node) {
    val_t result = value_null();
    for (node_t* child = node->child_begin; child; child = child->next) {
        result = eval(ctx, child);
        if (child->type == TYPE_RETURN) break;
    }
    return result;
}

static val_t eval_call(eval_ctx_t* ctx, node_t* call_node) {
    node_t* name_node = call_node->child_begin;
    if (!name_node || name_node->type != TYPE_IDENT) {
        return value_null();
    }
    
    fn_info_t* fn_info = env_find_function(ctx->env, name_node->value_token);
    if (!fn_info) {
        fprintf(stderr, "Unknown function: %.*s\n", 
                (int)name_node->value_token->size, name_node->value_token->data);
        return value_null();
    }
    
    // Create stack frame
    stack_push(ctx, fn_info);
    
    // Evaluate arguments and store in frame
    node_t* arg = name_node->next;
    for (size_t i = 0; i < fn_info->param_count && arg; i++, arg = arg->next) {
        val_t arg_val = eval(ctx, arg);
        // Store parameter in stack frame
        void* param_ptr = stack_get_var_ptr(ctx, fn_info->params[i].stack_offset);
        if (param_ptr && arg_val.type == TYPE_I64) {
            *(int64_t*)param_ptr = arg_val.value.i64;
        }
    }
    
    // Execute function body
    val_t result = eval(ctx, fn_info->body);
    
    // Clean up stack frame
    stack_pop(ctx);
    
    return result;
}

static val_t eval(eval_ctx_t* ctx, node_t* node) {
    if (!node) return value_null();
    
    switch (node->type) {
        case TYPE_NULL:
            return value_null();
            
        case TYPE_I64:
            return value_i64(node->value_i64);
            
        case TYPE_STR:
            return value_str(node->value_str);
            
        case TYPE_BOOL:
            return value_bool(node->value_i64 != 0);
            
        case TYPE_BLOCK:
            return eval_block(ctx, node);
            
        case TYPE_VAR: {
            // Variable declaration with optional initialization
            val_t init_val = value_i64(0);
            if (node->child_begin) {
                init_val = eval(ctx, node->child_begin);
            }
            
            if (node->var_info && !node->var_info->is_global) {
                // Store in stack frame
                void* var_ptr = stack_get_var_ptr(ctx, node->stack_offset);
                if (var_ptr && init_val.type == TYPE_I64) {
                    *(int64_t*)var_ptr = init_val.value.i64;
                }
            }
            
            return init_val;
        }
        
        case TYPE_IDENT: {
            // Variable reference - look up in current scope
            if (ctx->stack && ctx->stack->fn_info) {
                fn_info_t* fn = ctx->stack->fn_info;
                
                // Check function parameters
                for (size_t i = 0; i < fn->param_count; i++) {
                    if (token_equal(fn->params[i].name, node->value_token)) {
                        void* var_ptr = stack_get_var_ptr(ctx, fn->params[i].stack_offset);
                        if (var_ptr) {
                            return value_i64(*(int64_t*)var_ptr);
                        }
                    }
                }
                
                // Check function locals
                for (size_t i = 0; i < fn->local_count; i++) {
                    if (token_equal(fn->locals[i].name, node->value_token)) {
                        void* var_ptr = stack_get_var_ptr(ctx, fn->locals[i].stack_offset);
                        if (var_ptr) {
                            return value_i64(*(int64_t*)var_ptr);
                        }
                    }
                }
            }
            
            // Check global variables
            var_info_t* var_info = env_find_var(ctx->env, node->value_token);
            if (var_info && var_info->is_global) {
                // TODO: Implement global variable storage
                return value_i64(0);
            }
            
            fprintf(stderr, "Unknown variable: %.*s\n", 
                    (int)node->value_token->size, node->value_token->data);
            return value_null();
        }
        
        case TYPE_FN:
            // Function definition - just return null, function is already registered
            return value_null();
            
        case TYPE_CALL:
            return eval_call(ctx, node);
        
        case TYPE_RETURN:
            if (node->child_begin) {
                return eval(ctx, node->child_begin);
            }
            return value_null();
            
        case TYPE_ADD: {
            val_t left = eval(ctx, node->child_begin);
            val_t right = eval(ctx, node->child_begin->next);
            if (left.type == TYPE_I64 && right.type == TYPE_I64) {
                return value_i64(left.value.i64 + right.value.i64);
            }
            return value_null();
        }
        
        case TYPE_SUB: {
            val_t left = eval(ctx, node->child_begin);
            val_t right = eval(ctx, node->child_begin->next);
            if (left.type == TYPE_I64 && right.type == TYPE_I64) {
                return value_i64(left.value.i64 - right.value.i64);
            }
            return value_null();
        }
        
        case TYPE_MUL: {
            val_t left = eval(ctx, node->child_begin);
            val_t right = eval(ctx, node->child_begin->next);
            if (left.type == TYPE_I64 && right.type == TYPE_I64) {
                return value_i64(left.value.i64 * right.value.i64);
            }
            return value_null();
        }
        
        case TYPE_DIV: {
            val_t left = eval(ctx, node->child_begin);
            val_t right = eval(ctx, node->child_begin->next);
            if (left.type == TYPE_I64 && right.type == TYPE_I64 && right.value.i64 != 0) {
                return value_i64(left.value.i64 / right.value.i64);
            }
            return value_null();
        }
        
        case TYPE_EQ: {
            val_t left = eval(ctx, node->child_begin);
            val_t right = eval(ctx, node->child_begin->next);
            if (left.type == TYPE_I64 && right.type == TYPE_I64) {
                return value_bool(left.value.i64 == right.value.i64);
            }
            return value_bool(false);
        }
        
        case TYPE_ASSIGN: {
            // Assignment: var = expr
            node_t* var_node = node->child_begin;
            node_t* expr_node = node->child_begin->next;
            
            if (var_node->type != TYPE_IDENT) {
                fprintf(stderr, "Assignment to non-variable\n");
                return value_null();
            }
            
            val_t expr_val = eval(ctx, expr_node);
            
            // Find the variable in current scope and assign to it
            if (ctx->stack && ctx->stack->fn_info) {
                fn_info_t* fn = ctx->stack->fn_info;
                
                // Check function parameters
                for (size_t i = 0; i < fn->param_count; i++) {
                    if (token_equal(fn->params[i].name, var_node->value_token)) {
                        void* var_ptr = stack_get_var_ptr(ctx, fn->params[i].stack_offset);
                        if (var_ptr && expr_val.type == TYPE_I64) {
                            *(int64_t*)var_ptr = expr_val.value.i64;
                        }
                        return expr_val;
                    }
                }
                
                // Check function locals
                for (size_t i = 0; i < fn->local_count; i++) {
                    if (token_equal(fn->locals[i].name, var_node->value_token)) {
                        void* var_ptr = stack_get_var_ptr(ctx, fn->locals[i].stack_offset);
                        if (var_ptr && expr_val.type == TYPE_I64) {
                            *(int64_t*)var_ptr = expr_val.value.i64;
                        }
                        return expr_val;
                    }
                }
            }
            
            fprintf(stderr, "Assignment to unknown variable: %.*s\n", 
                    (int)var_node->value_token->size, var_node->value_token->data);
            return value_null();
        }
        
        case TYPE_SYSCALL_DEBUG01: {
            val_t arg = eval(ctx, node->child_begin);
            if (arg.type == TYPE_I64) {
                printf("%c", (char)arg.value.i64);
                fflush(stdout);
            }
            return value_null();
        }
        
        default:
            fprintf(stderr, "Eval not implemented for node type: %d\n", node->type);
            return value_null();
    }
}

// Main execution
int main(int argc, char* argv[]) {
    const char* filename = argc > 1 ? argv[1] : LKJSCRIPT_PATH;
    
    // Read source file
    char* source = file_read(filename);
    if (!source) {
        fprintf(stderr, "Failed to read source file: %s\n", filename);
        return 1;
    }
    
    // Tokenize
    token_t* tokens = tokenize(source);
    if (!tokens) {
        fprintf(stderr, "Tokenization failed\n");
        free(source);
        return 1;
    }
    
    // Parse
    node_t* program = parse_program(tokens);
    if (!program) {
        fprintf(stderr, "Parsing failed\n");
        free(source);
        return 1;
    }
    
    // Initialize evaluation context
    eval_ctx_t ctx = {0};
    ctx.env = &g_global_env;
    
    // Execute program
    val_t result = eval(&ctx, program);
    
    // Cleanup
    free(source);
    
    return result.type == TYPE_I64 ? (int)result.value.i64 : 0;
}