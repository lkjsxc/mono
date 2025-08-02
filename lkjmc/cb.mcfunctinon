# comment, <main>

# move, 02, 01, 02

# comment, <input>

# comment, kind3_tick
execute as @a[nbt={SelectedItem:{tag:{lkjmc:{k03:1b}}}}] positioned as @s run summon area_effect_cloud ~ ~ ~ {Tags:["a1"]}
execute as @e[type=area_effect_cloud,tag=a1] positioned as @s run data modify entity @s Pos set from entity @p SelectedItem.tag.lkjmc.cbpos
execute positioned as @e[type=area_effect_cloud,tag=a1] run data modify block ~ ~ ~ auto set value 1b
kill @e[type=area_effect_cloud,tag=a1]

# comment, kind4_tick
execute as @e[type=item,tag=kind4] positioned as @s facing entity @p[distance=..5,scores={talked=1..}] eyes positioned ^ ^ ^1 rotated as @p[distance=..5,scores={talked=1..}] positioned ^ ^ ^1 if entity @s[distance=..0.2] run data modify entity @s Item.tag.lkjmc.k06 set value 1b


# comment, </input>

# comment, tag_add
tag @e[type=item,tag=!cmded] add i1
tag @e[type=item,tag=i1,nbt={Item:{tag:{lkjmc:{k00:1b}}}}] add kind0
tag @e[type=item,tag=i1,tag=kind0,nbt={Item:{tag:{lkjmc:{k01:1b}}}}] add kind1
tag @e[type=item,tag=i1,tag=kind1,nbt={Item:{tag:{lkjmc:{k02:1b}}}}] add kind2
tag @e[type=item,tag=i1,tag=kind1,nbt={Item:{tag:{lkjmc:{k04:1b}}}}] add kind4
tag @e[type=item,tag=i1,tag=!kind0] add vanilla

# comment, vanilla_init
execute as @e[type=item,tag=i1,tag=vanilla] run data modify entity @s Item.tag.lkjmc.kind append value 0s
tag @e[type=item,tag=i1,tag=vanilla] add kind0

# comment, kind4_init
execute positioned as @e[type=item,tag=i1,tag=kind4] run summon villager ~ ~-0.66 ~ {NoGravity:1b,Silent:1b,Invulnerable:1b,NoAI:1b,CanPickUpLoot:0b,ActiveEffects:[{Id:14b,Amplifier:0b,Duration:2147483647,ShowParticles:0b}]}

# comment, kind2_tick, kind6_tick
tag @e[type=item,tag=kind2] add i2
tag @e[type=item,nbt={Item:{tag:{lkjmc:{k06:1b}}}}] add i2
execute as @e[type=item,tag=i2] run data modify entity @s Item.tag.lkjmc.tmp set from entity @s Pos
execute as @e[type=item,tag=i2] run data modify entity @s Pos set from entity @s Item.tag.lkjmc.cbpos
execute positioned as @e[type=item,tag=i2] run data modify block ~ ~ ~ auto set value 1b
execute as @e[type=item,tag=i2] run data modify entity @s Pos set from entity @s Item.tag.lkjmc.tmp
execute as @e[type=item,tag=i2] run data remove entity @s Item.tag.lkjmc.tmp
execute as @e[type=item,tag=i2] run data remove entity @s Item.tag.lkjmc.k06
tag @e[type=item,tag=i2] remove i2

# comment, clear
scoreboard players reset @a[scores={talked=1..}] talked
tag @e[type=item,tag=i1] add cmded
tag @e[type=item,tag=i1] remove i1

# comment, </main>



# comment, <test>

# move, 02, 01, 05
execute store result block ~ ~ ~ auto byte 0 as @e[type=item,nbt={Item:{tag:{lkjmc:{cbpos:[2.0d,1.0d,5.0d]}}}}] run say Good Morning World!!

# move, 02, 01, 08
summon item ~ ~1 ~ {NoGravity:1b,Age:-32768,PickupDelay:32767,Item:{id:"minecraft:iron_axe",Count:1b,tag:{lkjmc:{k00:1b,k01:1b,k04:1b,cbpos:[2.0d,1.0d,5.0d]}}}}

# comment, </test>



# comment, <python>

# comment, # move, 02, 01, 11
# comment, say test
# comment, execute unless data block ~ ~ ~ {Command:""} run data merge block ~ ~-1 ~ {Command:""}

# comment, </python>