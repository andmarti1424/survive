# survive
terminal survival game

A little survival game for linux terminal.
Requires ncurses and threads.

Build with:
gcc -lncurses -pthread survive.c -o survive


## Story
You were driving in your car, trying to get to San Miguel de Tucumán in an alternative road that you were advised not to drive into.
You insisted on taking it because your hurry to get there sooner.
A heavy storm started to take place and you couldn't see in more than 3 feet away.
Suddenly a coyote crossed the road. You managed to avoid it by performing a desperate maneuver, but you lost control of the vehicle,
getting it to toppled over.
Your mission is to survive and get to a safe place.

Estabas manejando en tu auto, tratando de llegar a San Miguel de Tucumán en un camino alternativo que te aconsejaron no conducir.
Ignoraste la advertencia dada tu prisa para llegar más pronto a destino.
Una fuerte tormenta comenzó a ocurrir y no te permitía visualizar nada a más de 3 metros de distancia.
De repente, llegas a ver que un coyote se cruzó en el camino. Lograste evitarlo realizando una maniobra desesperada,
pero perdiste el control del vehículo, y el mismo volcó.
Tu misión es sobrevivir y encontrar un lugar seguro.

GAME ENDS WHEN:

player gets rescued

player gets to safe place

player dies

Player can die because of dehydratation, hypothermia, because of starving, or can get killed.

Player can get rescued at anytime. The longer you travel, the more probable is to get rescued, or to get to safe place.
