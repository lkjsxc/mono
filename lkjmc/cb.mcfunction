# move, 08, 01, 02

# comment, kind0
tag @e[type=item,tag=!cmded,nbt={Item:{tag:{lkjmc:{kind:[0s]}}}}] add kind0

# comment, vanilla
tag @e[type=item,tag=!cmded,tag=!kind0] add vanilla

# comment, kind1
tag @e[type=item,tag=kind0,nbt={Item:{tag:{lkjmc:{kind:[1s]}}}}] add kind1

# comment, kind2
tag @e[type=item,tag=kind1,nbt={Item:{tag:{lkjmc:{kind:[2s]}}}}] add kind2

# comment, kind3
execute as @a[nbt={SelectedItem:{tag:{lkjmc:{kind:[3s]}}}}] positioned as @s run summon area_effect_cloud ~ ~ ~ {Tags:["a1"]}
execute as @e[type=area_effect_cloud,tag=a1] positioned as @s run data modify entity @s Pos set from entity @p SelectedItem.tag.lkjmc.cbpos
execute positioned as @e[type=area_effect_cloud,tag=a1] run data modify block ~ ~ ~ auto set value 1b
kill @e[type=area_effect_cloud,tag=a1]

# move, 02, 01, 05
execute store result block ~ ~ ~ auto byte 0 as @a[nbt={SelectedItem:{tag:{lkjmc:{cbpos:[2.0d,1.0d,5.0d]}}}}] run say Good Morning World!!

# move, 02, 01, 08
summon item ~ ~1 ~ {Item:{id:"minecraft:iron_axe",Count:1b,tag:{lkjmc:{kind:[0s,1s,3s],cbpos:[2.0d,1.0d,5.0d]}}}}