yb	-24
xv	0
picn	"q1/sbar.q1p"

yb	-48
picn	"q1/ibar.q1p"

yb	-24
xv	24
q1_rnum

xv	136
q1_hnum

xv	112
q1_face

xv	248
q1_anum

if $ammo_icon uint16
	xv	224
	pic	$ammo_icon
endif

if $armor_icon uint16
	xv	0
	pic	$armor_icon
endif

yb	-48

xv	10
q1_ammo	shells

xv	58
q1_ammo	nails

xv	106
q1_ammo	rockets

xv	154
q1_ammo	cells

yb	-40
xv	0
q1_weapon	1	"q1/inv_shotgun.q1p"	"q1/inv2_shotgun.q1p"

xv	24
q1_weapon	2	"q1/inv_sshotgun.q1p"	"q1/inv2_sshotgun.q1p"

xv	48
q1_weapon	3	"q1/inv_nailgun.q1p"	"q1/inv2_nailgun.q1p"

xv	72
q1_weapon	4	"q1/inv_snailgun.q1p"	"q1/inv2_snailgun.q1p"

xv	96
q1_weapon	5	"q1/inv_rlaunch.q1p"	"q1/inv2_rlaunch.q1p"

xv	120
q1_weapon	6	"q1/inv_srlaunch.q1p"	"q1/inv2_srlaunch.q1p"

xv	144
q1_weapon	7	"q1/inv_lightng.q1p"	"q1/inv2_lightng.q1p"

xv	192
q1_item	8	"q1/sb_quad.q1p"

xv	208
q1_item	9	"q1/sb_invuln.q1p"

xv	224
q1_item	10	"q1/sb_suit.q1p"

xv	240
q1_item	11	"q1/sb_invis.q1p"