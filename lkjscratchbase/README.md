# global_val
1 ~ 1024

## register
0001. = int zero
0002. = int stack_pointer
0003. = int base_pointer
0004. = int global_pointer

## tmp_any
0005. = any tmp01
0006. = any tmp02
0007. = any tmp03
0008. = any tmp04
0009. = any tmp05
0010. = any tmp06
0011. = any tmp07
0012. = any tmp08

## tmp_vec
0017. = vec<any> tmp_vec1
0018. = vec<any> tmp_vec2
0019. = vec<any> tmp_vec3
0020. = vec<any> tmp_vec4

## node
0033. = vec<node> node_data
0034. = vec<node*> node_free

## obj
0035. = vec<obj> obj_data
0036. = vec<obj*> obj_free
0037. = map<uid, obj*> obj_all

## clone
0049. = vec<obj*> obj_clone

## table
0065. = vec<typetable> type_table

## time
0257. = float utc_started
0258. = float utc_now
0259. = float utc_delta

## type
### debug
0513. = map<uid, obj*> debug01
0514. = map<uid, obj*> debug02
0515. = map<uid, obj*> global

## global_obj
0769. = obj* obj_origin
0770. = obj* obj_camera

# obj

## all type
00. int type
01. float uid
02. float created_at

## link
03. obj* next
04. obj* prev

## visible
17. float layer_priority
18. float pos_x
19. float pos_y
20. float scale
21. float dir
22. str costume
23. float visibility
24. int color

# TODO

## clone
-   camera_init
-   clone_tick

## 