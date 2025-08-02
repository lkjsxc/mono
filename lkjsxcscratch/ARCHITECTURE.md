# ARCHITECTURE

## global

### register
000000. null
000001. zero
000002. stack_pointer
000003. base_pointer
000004. obj_pointer
000005. undefined
000006. undefined
000007. undefined

### node
000008. node_data
000009. node_freelist

### obj
000010. obj_data
000011. obj_freelist

### clone
000012. clone_data
000013. clone_freelist

### tmp
000016. tmp01
000017. tmp02
000018. tmp03
000019. tmp04
000020. tmp05
000021. tmp06
000022. tmp07
000023. tmp08
000024. tmp09
000025. tmp10
000026. tmp11
000027. tmp12
000028. tmp13
000029. tmp14
000030. tmp15
000031. tmp16

### node_tmp
000256. tmp01
000257. tmp02
000258. tmp03
000259. tmp04

### obj_tmp
000260. tmp01
000261. tmp02
000262. tmp03
000263. tmp04

### clone_tmp
000264. tmp01
000265. tmp02
000266. tmp03
000267. tmp04

### obj_type
001024. type_all
001025. type_global
001026. test01
001027. test02
001028. test03
001029. test04

## obj
00. obj_id
01. obj_type
02. undefined
03. undefined
04. clone
05. undefined
06. undefined
07. undefined

## clone
00. enable
01. undefined
02. undefined
03. undefined
04. undefined
05. undefined
06. undefined
07. undefined
08. pos_x
09. pos_y
10. scale
11. dir
12. undefined
13. undefined
14. undefined
15. undefined