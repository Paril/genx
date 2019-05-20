yb	-24

// health
xv	0
hnum
if $q2.health_icon uint16
	xv	50
	pic	$q2.health_icon
endif

// ammo
if $ammo_icon uint16
	xv	100
	anum
	xv	150
	pic	$ammo_icon
endif

// armor
if $armor_icon uint16
	xv	200
	rnum
	xv	250
	pic	$armor_icon
endif

// selected item
if $q2.selected_icon uint16
	xv	296
	pic 	$q2.selected_icon
endif

yb	-50

// picked up item
if $q2.pickup_icon uint16
	xv	0
	pic	$q2.pickup_icon
	xv	26
	yb	-42
	stat_string	$q2.pickup_string
	yb	-50
endif

// timer
if $q2.timer_icon uint16
	xv	262
	num	2	$q2.timer	uint16
	xv	296
	pic	$q2.timer_icon
endif

//  help / weapon icon
if $q2.help_icon uint16
	xv	148
	pic	$q2.help_icon
endif