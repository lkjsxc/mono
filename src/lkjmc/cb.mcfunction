# comment, main

# move, 08, 01, 02

# comment, i1_init
tag @e[type=item,tag=!cmded] add i1

# comment, kind0_init
tag @e[type=item,tag=i1,nbt={Item:{tag:{lkjmc:{kind:[0s]}}}}] add kind0

# comment, kind1_init
tag @e[type=item,tag=i1,tag=kind0,nbt={Item:{tag:{lkjmc:{kind:[1s]}}}}] add kind1

# comment, kind2_init
tag @e[type=item,tag=i1,tag=kind1,nbt={Item:{tag:{lkjmc:{kind:[2s]}}}}] add kind2

# comment, kind4_init
tag @e[type=item,tag=i1,tag=kind1,nbt={Item:{tag:{lkjmc:{kind:[4s]}}}}] add kind4
execute positioned as @e[type=item,tag=i1,tag=kind4] run summon villager ~ ~-0.66 ~ {NoGravity:1b,Silent:1b,Invulnerable:1b,NoAI:1b,CanPickUpLoot:0b,ActiveEffects:[{Id:14b,Amplifier:0b,Duration:2147483647,ShowParticles:0b}]}

# comment, kind3_tick
execute as @a[nbt={SelectedItem:{tag:{lkjmc:{kind:[3s]}}}}] positioned as @s run summon area_effect_cloud ~ ~ ~ {Tags:["a1"]}
execute as @e[type=area_effect_cloud,tag=a1] positioned as @s run data modify entity @s Pos set from entity @p SelectedItem.tag.lkjmc.cbpos
execute positioned as @e[type=area_effect_cloud,tag=a1] run data modify block ~ ~ ~ auto set value 1b
kill @e[type=area_effect_cloud,tag=a1]

# comment, kind4_tick
execute as @e[type=item,tag=kind4] positioned as @s facing entity @p[distance=..5,scores={talked=1..}] eyes positioned ^ ^ ^1 rotated as @p[distance=..5,scores={talked=1..}] positioned ^ ^ ^1 if entity @s[distance=..0.2] run say test

# comment, vanilla
tag @e[type=item,tag=i1,tag=!kind0] add vanilla
execute as @e[type=item,tag=i1,tag=vanilla] run data modify entity @s Item.tag.lkjmc.kind append value 0s

# comment, clear
scoreboard players reset @a[scores={talked=1..}] talked
tag @e[type=item,tag=i1] add cmded
tag @e[type=item,tag=i1] remove i1



# comment, test

# move, 02, 01, 05
execute store result block ~ ~ ~ auto byte 0 as @a[nbt={SelectedItem:{tag:{lkjmc:{cbpos:[2.0d,1.0d,5.0d]}}}}] run say Good Morning World!!

# move, 02, 01, 08
summon item ~ ~1 ~ {NoGravity:1b,Item:{id:"minecraft:iron_axe",Count:1b,tag:{lkjmc:{kind:[0s,1s,4s],cbpos:[2.0d,1.0d,5.0d]}}}}



# comment, python

# comment, # move, 05, 01, 08
# comment, say test
# comment, execute unless data block ~ ~ ~ {Command:""} run data merge block ~ ~-1 ~ {Command:""}