ACTIONS.DOC - Docs used for creating social commands

Format:

<action#> <see_if_visible> <min_pos_victim>
Action to character with no victim.
Action to room with no victim.
Action to character with victim.
Action to room with victim.
Action to victim when victim found.
Action to character when victim not found.
Action to character when character is victim.
Action to room when character is victim.

Variables for actions:

$n - character name doing action
$N - victim name of action

$m - him/her/it of character doing action
$M - him/her/it of victim

$s - his/her/its of character doing action
$S - his/her/its of victim

$e - he/she/it of character doing action
$E - he/she/it of victim
 
<action#> - defined command number
<see_if_visible> - 0 = action seen even if char is invis (Someone hugs you.)
                   1 = action not seen if char is invis
<min_pos_victim> - minimum position victim must be in for action

 POSITION_DEAD       0
 POSITION_MORTALLYW  1
 POSITION_INCAP      2
 POSITION_STUNNED    3
 POSITION_SLEEPING   4
 POSITION_RESTING    5
 POSITION_SITTING    6
 POSITION_FIGHTING   7
 POSITION_STANDING   8
 POSITION_SWIMMING   9
 POSITION_RIDING    10

Examples:

# - used in place of message so nothing is echoed

9 0 5
You pucker up, looking for a victim.
#
You give $M a big, sloppy, wet kiss.
$n gives $N a big, sloppy, wet kiss.
$n gives you a big, sloppy, wet kiss.
Never around when required.
You pucker up, and drool down your front.
$n vainly attempts to kiss $mself.

22 0 0
You bounce around with excitement!
$n bounces around, really excited!
You bounce in circles around $M.
$n bounces in circles around $N.
$n bounces in circles around you.
You notice that the person is not here.
You bounce your body on the ground a few times, knocking yourself senseless.
$n bounces $s body on the ground.  Ooh, that must really hurt!

49 0 5
Hug who?
#
You give $M a great big, warm hug.
$n hugs $N.
$n gives you a great big, warm hug.
Sorry, friend, I can't see that person here.
You hug yourself.
$n hugs $mself.

Valkur, November 26, 1994


