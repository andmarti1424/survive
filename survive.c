/*
 *  You were driving in your car, trying to get to San Miguel de Tucumán in an alternative road that you were advised not to drive into.
 *  You insisted on taking it because your hurry to get there sooner.
 *  A heavy storm started to take place and you couldn't see in more than 3 feet away.
 *  Suddenly a coyote crossed the road. You managed to avoid it by performing a desperate maneuver, but you lost control of the vehicle,
 *  getting it to toppled over.
 *  Your mission is to survive and get to a safe place.
 *
 *  Estabas manejando en tu auto, tratando de llegar a San Miguel de Tucumán en un camino alternativo que te aconsejaron no conducir.
 *  Ignoraste la advertencia dada tu prisa para llegar más pronto a destino.
 *  Una fuerte tormenta comenzó a ocurrir y no te permitía visualizar nada a más de 3 metros de distancia.
 *  De repente, llegas a ver que un coyote se cruzó en el camino. Lograste evitarlo realizando una maniobra desesperada,
 *  pero perdiste el control del vehículo, y el mismo volcó.
 *  Tu misión es sobrevivir y encontrar un lugar seguro.
 *
 *  GAME ENDS WHEN:
 *  player gets rescued
 *  player gets to safe place
 *  player dies
 *
 *  Player can die because of hydratation, hipotermia, because of starving, or can get killed.
 *  Player can get rescued at anytime. The longer you travel, the more probable is to get rescued, or to get to safe place.
 *
 */

// includes
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// defines
#define m_increment                      5  // gameplay minutes that increments in each loop
#define s_loop                           5  // realtime seconds in each game loop

#define NIGHT_START_HOUR                20  // night starts at 8PM
#define DAY_START_HOUR                   8
#define DAYS_WITHOUT_FOOD_TO_STARV      15
#define DAYS_WITHOUT_WATER_TO_DIE      1.5
#define DESIDERED_DAILY_HOURS_OF_REST    8
#define HOURS_TO_REST                    4
#define HOURS_TO_REST_WHEN_NO_STAMINA   14
#define MAX_HOURS_WITHOUT_REST          30
#define BEANS_CAN_ENERGY               300
#define MILES_FOR_SAFEPLACE            300  // miles to travel to get to safe place
#define FIRE_WEIGHT_FOR_RESCUE        0.10  // fire weight over miles_travelled to increase chance of getting rescued
#define HYDRATATION_DURING_REST        270
#define MILES_PER_HOUR                   3  // miles that player can walk in an hour
#define HOUR_DECREASE_STAMINA_DURING_NAVIGATION        50
#define HOUR_DECREASE_HYDRATATION_DURING_NAVIGATION    40

// structures
struct datetime {
    int day;
    int minutes;
};

struct health {
    float heat;
    float hydratation;
    float energy;
    float stamina;
};

short miles_travelled;             // miles travelled

// each scenario that we can move to have
// a 0-2 grade of rain_probability, fish_availability, hunt_capability
// 0 means none, 1 normal, 2 a lot.
struct scenario {
    int miles; // miles to travel (2, 4, 6, 8 or 10)
    int rain_probability;
    int fish_availability;
    int hunt_capability;
};

struct environment {
    short temp;
    int wood_available;
    float rain_probability;        // probability of rain in a day (0 to 100)
    int hunt_capability;           // hunt_capability (0 to 100)
    int fish_availability;         // fish availability (0 to 100)
    short minutes_rain_left;
    short minutes_since_last_rain;
    int fire;                      // current fire status. 0 means no fire. different to 0 means fire left in minutes
    //short rescue_probability;      // rescue_probability in current scenario
    struct scenario next_scenario1;
    struct scenario next_scenario2;
};

struct goods {
    short tinder;
    short wood;
    short rope;
    short bait;
};

struct inventory {
    short empty_bottles;
    short filled_safe_bottles;
    short filled_unsafe_bottles;
    short matches;
    short rain_catcher;
    short rain_catcher_fills;
    short beans_can;
    short squirrel;
    short mushrooms;
    short crickets;
};

// prototipos
void forward_time(short minutes);
void get_wood();
void drink_water(short type);
void boil_water();
void fill_bottles_water_with_rain_catcher_fills();
void increase_heat(float t);
void decrease_heat(float t);
void increase_hydratation(float t);
void decrease_hydratation(float t);
void increase_energy(float t);
void decrease_energy(float t);
void increase_stamina(float t);
void decrease_stamina(float t);
void rest(short hours);
void start_fire();
void feed_wood_to_fire();
void eat_beans_can();
void handle_rain();
void * show_progressbar(void * p);

// define global variables
int shall_quit = 0;
struct datetime datetime;
struct environment environment;
struct health health;
struct goods goods;
struct inventory inventory;
pthread_t forward_time_thread, input_thread, progress_bar_thread, rain_thread;

int in_pause = 0;
int in_travel_menu = 0;

/* *****************************************************************************
   FUNCIONES DE VISTA
   *****************************************************************************/

void print_main_menu() {
    mvprintw(13, 0, "Get Wood                                [d]   (1 hour) ");
    wclrtoeol(stdscr);
    mvprintw(14, 0, "Start fire                              [f]   (15 min)");
    wclrtoeol(stdscr);
    mvprintw(15, 0, "Feed wood to fire                       [e]");
    wclrtoeol(stdscr);

    mvprintw(16, 0, "Drink water                             [w]");
    wclrtoeol(stdscr);
    mvprintw(17, 0, "Fill empty bottles with rain catcher    [y]");
    wclrtoeol(stdscr);
    mvprintw(18, 0, "Boil water                              [b]   (15 min)");
    wclrtoeol(stdscr);

    mvprintw(19, 0, "Eat beans can                           [o]");
    wclrtoeol(stdscr);
    mvprintw(20, 0, "Rest                                    [r]   (4 hours)");
    wclrtoeol(stdscr);
    mvprintw(21, 0, "Navigate                                [n]   (2 hours)");
    wclrtoeol(stdscr);
    // mvprintw(22, 0, "Shelter                                 [s]");
    // wclrtoeol(stdscr);
    // mvprintw(23, 0, "Crafting                                [c]");
    // wclrtoeol(stdscr);
    mvprintw(24, 0, "Test (forward time)                     [t]");
    wclrtoeol(stdscr);
    mvprintw(25, 0, "Show travel options                     [m]");
    wclrtoeol(stdscr);
    mvprintw(26, 0, "Travel to destination 1                 [A]");
    wclrtoeol(stdscr);
    mvprintw(27, 0, "Travel to destination 2                 [B]");
    wclrtoeol(stdscr);
    //mvprintw(28, 0, "Food                                    [o]");
    //wclrtoeol(stdscr);
}


void * show_progressbar(void * p) {
    int j;
    wmove(stdscr, 0, 0);
    wclrtobot(stdscr);
    refresh();
    float f = 800000 / COLS;
    for (j=0; j<COLS; j++) {
        mvprintw(LINES-1, j, "*");
        wclrtoeol(stdscr);
        refresh();
        usleep(f);
    }
    mvprintw(LINES-1, 0, " ");
    wclrtoeol(stdscr);
    refresh();
}

void print_day_hour_miles_temp() {
    mvprintw(0, 0, "Day: %d", datetime.day);
    //mvprintw(0, 0, "Day: %d                                  hour: %d         minutes: %d", datetime.day, datetime.minutes/60, datetime.minutes);
    wclrtoeol(stdscr);

    if (datetime.minutes >= NIGHT_START_HOUR*60) // > 20hs
        mvprintw(1, 0, "Hours darkness remaining: %d", (24+DAY_START_HOUR)-datetime.minutes/60);
    else if (datetime.minutes < 8*60)
        mvprintw(1, 0, "Hours darkness remaining: %d", DAY_START_HOUR-datetime.minutes/60);
    else
        mvprintw(1, 0, "Hours daylight remaining: %d", NIGHT_START_HOUR-datetime.minutes/60);
    wclrtoeol(stdscr);

    mvprintw(2, 0, "temp: %d", environment.temp);
    wclrtoeol(stdscr);
    mvprintw(3, 0, "miles travelled: %d", miles_travelled);
    wclrtoeol(stdscr);

    if (environment.minutes_since_last_rain < 0 && environment.minutes_since_last_rain > -60)
        mvprintw(5, 0, "raining for %d minutes", -environment.minutes_since_last_rain);
    else if (environment.minutes_since_last_rain < 0)
        mvprintw(5, 0, "raining for %d hours", -environment.minutes_since_last_rain / 60);
    wclrtoeol(stdscr);

    if (environment.fire && environment.fire < 60)
        mvprintw(6, 0, "we have fire. minutes left: %d", environment.fire);
    else if (environment.fire && environment.fire > 60)
        mvprintw(6, 0, "we have fire. hours left: %d", environment.fire/60);
    else
        move(6, 0);
    wclrtoeol(stdscr);
    return;
}

void print_health() {
    mvprintw(8, 0, "heat: %.0f", health.heat / 10);
    wclrtoeol(stdscr);
    mvprintw(9, 0, "hydratation: %.0f", health.hydratation / 10);
    wclrtoeol(stdscr);
    mvprintw(10, 0, "energy: %.0f", health.energy / 10);
    wclrtoeol(stdscr);
    mvprintw(11, 0, "stamina: %.0f", health.stamina / 10);
    wclrtoeol(stdscr);
}

void print_inventory() {
    mvprintw(9, 60, "Inventory:");
    wclrtoeol(stdscr);
    mvprintw(11, 60, "empty bottles: %d", inventory.empty_bottles);
    wclrtoeol(stdscr);
    mvprintw(12, 60, "filled safe bottles: %d", inventory.filled_safe_bottles);
    wclrtoeol(stdscr);
    mvprintw(13, 60, "filled unsafe bottles: %d", inventory.filled_unsafe_bottles);
    wclrtoeol(stdscr);
    mvprintw(14, 60, "matches: %d", inventory.matches);
    wclrtoeol(stdscr);
    mvprintw(15, 60, "rain catcher: %d", inventory.rain_catcher);
    wclrtoeol(stdscr);
    mvprintw(16, 60, "rain catcher fills: %d", inventory.rain_catcher_fills);
    wclrtoeol(stdscr);
    mvprintw(17, 60, "beans can: %d", inventory.beans_can);
    wclrtoeol(stdscr);
    mvprintw(18, 60, "squirrels: %d", inventory.squirrel);
    wclrtoeol(stdscr);
    mvprintw(19, 60, "crickets: %d", inventory.crickets);
    wclrtoeol(stdscr);
    mvprintw(20, 60, "mushrooms: %d", inventory.mushrooms);
    wclrtoeol(stdscr);
}

void print_goods() {
    mvprintw(22, 60, "tinder: %d", goods.tinder);
    wclrtoeol(stdscr);
    mvprintw(23, 60, "wood: %d", goods.wood);
    wclrtoeol(stdscr);
    mvprintw(24, 60, "rope: %d", goods.rope);
    wclrtoeol(stdscr);
    mvprintw(25, 60, "bait: %d", goods.bait);
    wclrtoeol(stdscr);
}

void show_status() {
    if (in_travel_menu) return;
    print_day_hour_miles_temp();
    print_health();
    print_main_menu();
    print_goods();
    print_inventory();
    refresh();
}

void debug(char * s, ...) {
    char t[100];
    va_list args;
    va_start(args, s);
    vsprintf(t, s, args);
    mvprintw(LINES - 1, 60, "%s", t);
    wclrtoeol(stdscr);
    refresh();
    va_end(args);
}
void msg(char * s) {
    mvprintw(LINES - 1, 0, "%s", s);
    wclrtoeol(stdscr);
    refresh();
}
/* *****************************************************************************
   FIN DE FUNCIONES DE VISTA
   *****************************************************************************/

























void check_end_of_game() {
    float end = (float) rand_lim(MILES_FOR_SAFEPLACE);

    // if fire is on, increase possibility of getting rescued.
    float fire_prob = 0.0;
    if (environment.fire)
        fire_prob = ((float) rand_lim((int) (FIRE_WEIGHT_FOR_RESCUE * 1000))) * 1.0 / 1000.0;
    //debug("fire_prob: %2.4f", fire_prob);
    if (miles_travelled * (1.0 + fire_prob) / MILES_FOR_SAFEPLACE > end) {
        mvprintw(0, 0, "Game over.");
        wclrtobot(stdscr);
        mvprintw(1, 0, "You got rescued by an old man who was hunting.");
        mvprintw(2, 0, "Press 'q' to quit the game.");
        refresh();
        shall_quit = 1;
        getch();
        //pthread_cancel(forward_time_thread);
    }
    return;
}

void check_health() {
    // if no stamina force to sleep
    if (! health.stamina) {
        mvwprintw(stdscr, 0, 0, "I AM SO TIRED. BETTER REST A LITTLE BIT..");
        wclrtobot(stdscr);
        refresh();
        sleep(1);
        rest(HOURS_TO_REST_WHEN_NO_STAMINA);
        return;
    }

    if ((! health.heat) || (! health.energy) || (! health.hydratation)) {
        mvprintw(0, 0, "I am dead.");
        wclrtoeol(stdscr);
        if (! health.heat)
            mvprintw(1, 0, "I died of hipotermia.");
        else if (! health.energy) // TODO chequear tiempo transcurrido desde ultima ingesta
            mvprintw(1, 0, "Did not eat anything from days..");
        else if (! health.hydratation)
            mvprintw(1, 0, "Did not drink water for days..");
        wclrtobot(stdscr);
        mvprintw(2, 0, "Press 'q' to quit the game.");
        refresh();
        shall_quit = 1;
        getch();
        //FIXME
        //pthread_cancel(forward_time_thread);
    }
    return;
}

void forward_time(short minutes) {
    datetime.minutes += minutes;
    if (datetime.minutes > 24*60) {
        datetime.minutes -= 24*60;
        datetime.day++;
    }

    // decrease_fire
    environment.fire -= minutes;
    if (environment.minutes_since_last_rain < 0 && rand_lim(1))
        environment.fire -= minutes;
    if (environment.fire < 0) environment.fire = 0;

    // decrease temp
    float j = 0;
    if (! environment.fire && datetime.minutes >= NIGHT_START_HOUR*60 && datetime.minutes < DAY_START_HOUR*60) j = (float) rand_lim(2);
    else if (! environment.fire && (datetime.minutes >= NIGHT_START_HOUR*60 || datetime.minutes < DAY_START_HOUR*60)) j = (float) rand_lim(1);
    decrease_heat(j * 50);
    j = 0;

    // increase temp
    if (environment.fire && (datetime.minutes >= NIGHT_START_HOUR*60 || datetime.minutes < DAY_START_HOUR*60)) j = (float) rand_lim(1);
    else if (environment.fire && datetime.minutes >= NIGHT_START_HOUR*60 && datetime.minutes < DAY_START_HOUR*60) j = (float) (rand_lim(2) + 1);
    increase_heat(j * 100);

    // bajo energia
    decrease_energy((1000.0 / (DAYS_WITHOUT_FOOD_TO_STARV * 60 * 24)) * minutes); // 15 days without food
    //decrease_energy(1000.0 / 21600 * minutes ); // 15 days without food

    // bajo hidratacion
    decrease_hydratation((1000.0 / (DAYS_WITHOUT_WATER_TO_DIE * 60 * 24)) * minutes); // 1.5 days without water
    //decrease_hydratation(1000.0 / 2160 * minutes); // 1.5 days without water

    // bajo stamina
    decrease_stamina((1000.0 / (MAX_HOURS_WITHOUT_REST * 60)) * minutes); // 24 hs max without rest
    //decrease_stamina(1000.0 / 1440 * minutes); // 24 hs max without rest

    return;
}

void * rain_thread_function(void * p) {
    while (! shall_quit) {
        if (in_pause) continue;
        sleep(60/m_increment); // sleep one gameplay hour
        handle_rain();
    }
}

void * time_thread_function(void * p) {
    while (! shall_quit) {
        if (in_pause) continue;
        show_status();
        sleep(s_loop);
        forward_time(m_increment);
        check_health();
        check_end_of_game();
    }
}

void create_next_scenarios() {
    environment.next_scenario1.miles = rand_lim(3) * 2 + 2;
    environment.next_scenario1.rain_probability = rand_lim(2);
    environment.next_scenario1.fish_availability = rand_lim(2);
    environment.next_scenario1.hunt_capability = rand_lim(2);
    environment.next_scenario2.miles = rand_lim(3) * 2 + 2;
    environment.next_scenario2.rain_probability = rand_lim(2);
    environment.next_scenario2.fish_availability = rand_lim(2);
    environment.next_scenario2.hunt_capability = rand_lim(2);
}


void show_moving_options() {
    mvwprintw(stdscr, 0, 0, "Possible scenarios to move to:");
    wclrtobot(stdscr);
    mvwprintw(stdscr, 1, 0, "Scenario 1");
    mvwprintw(stdscr, 2, 0, "\tMiles ahead: %d", environment.next_scenario1.miles);
    mvwprintw(stdscr, 3, 0, "\tRain probability: %d", environment.next_scenario1.rain_probability);
    mvwprintw(stdscr, 4, 0, "\tFish availability: %d", environment.next_scenario1.fish_availability);
    mvwprintw(stdscr, 5, 0, "\tHunt capability: %d", environment.next_scenario1.hunt_capability);
    mvwprintw(stdscr, 7, 0, "Scenario 2");
    mvwprintw(stdscr, 8, 0, "\tMiles ahead: %d", environment.next_scenario2.miles);
    mvwprintw(stdscr, 9, 0, "\tRain probability: %d", environment.next_scenario2.rain_probability);
    mvwprintw(stdscr, 10, 0, "\tFish availability: %d", environment.next_scenario2.fish_availability);
    mvwprintw(stdscr, 11, 0, "\tHunt capability: %d", environment.next_scenario2.hunt_capability);
    mvwprintw(stdscr, 13, 0, "Press a key to return.");
    refresh();
    getch();
    wmove(stdscr, 0, 0);
    wclrtobot(stdscr);
    in_travel_menu=0;
    show_status();
    return;
}

void navigate(short hours) {
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    decrease_stamina((1000.0 / HOUR_DECREASE_STAMINA_DURING_NAVIGATION) * hours);
    decrease_hydratation((1000.0 / HOUR_DECREASE_HYDRATATION_DURING_NAVIGATION) * hours);
    forward_time(60 * hours);
    int n = rand_lim(13);

    if (n == 1 && environment.hunt_capability == 1) {
        inventory.squirrel++;
        msg("found a squirrel");
    } else if (n < 3 && environment.hunt_capability > 0) {
        inventory.squirrel++;
        msg("found a squirrel");
    } else if (n == 3) {
        inventory.matches++;
        msg("found a box of matches");
    } else if (n == 4 || n == 5 || n == 6) {
        goods.bait++;
        msg("found bait");
    } else if (n == 7) {
        goods.rope++;
        msg("found a rope");
    } else if (n == 8) {
        inventory.mushrooms++;
        msg("found mushrooms");
    } else if (n == 9) {
        inventory.crickets++;
        msg("found crickets");
    } else if (n == 10) {
        inventory.beans_can++;
        msg("found a can of beans");
    } else if (n == 11 || n == 12) {
        goods.tinder++;
        msg("found tinder");
    } else {
        msg("found nothing");
    }
    show_status();
    return;
}

void rest(short hours) {
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    increase_stamina((1000.0 / DESIDERED_DAILY_HOURS_OF_REST) * hours);
    increase_hydratation((HYDRATATION_DURING_REST / DESIDERED_DAILY_HOURS_OF_REST) * hours);
    forward_time(60 * hours);
    show_status();
    return;
}



// type 0 - we drink filled_bottles
void drink_water(short type) {
    if (type == 0 && inventory.filled_safe_bottles && health.hydratation == 1000)
        msg("Not thirsty. Shouldn't waste water!");
    else if (type == 0 && inventory.filled_safe_bottles) {
        inventory.filled_safe_bottles--;
        inventory.empty_bottles++;
        //pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
        //pthread_join(progress_bar_thread, NULL);
        //forward_time(60);
        increase_hydratation(270);   //2160/8
        msg("Drunk water!");
    } else if (type == 0) {
        msg("It appears there are no more bottles filled with water !");
    }
    show_status();
    return;
}

void get_wood() {
    if (environment.wood_available) {

        pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
        pthread_join(progress_bar_thread, NULL);

        forward_time(60);

        // TODO analizar si se tiene stamina para cortar leña
        // TODO analizar si se esta en horario nocturno


        int w = 1 + rand_lim(2);
        goods.wood += w;
        environment.wood_available -= w;

        // se sube temperatura corporal dependiendo de la temperatura exterior
        environment.temp > 17 ? increase_heat(5) : increase_heat(3);

        // bajar energia e hidratacion
        decrease_hydratation(4);
        decrease_energy(3);

        msg("Got wood!");
    } else {
        msg("It appears there is no wood left in here !");
    }
    check_health();
    show_status();
    return;
}

// option = number of scenario to move to
void travel(int option) {
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);

    int miles = option == 1 ? environment.next_scenario1.miles :  environment.next_scenario2.miles;
    int rain_prob = option == 1 ? environment.next_scenario1.rain_probability :  environment.next_scenario2.rain_probability;

    miles_travelled += miles;

    forward_time(miles / (MILES_PER_HOUR));

    // TODO check here if we arrived to safe place!!!

    // bajar energia e hidratacion
    decrease_hydratation(4);
    decrease_energy(3);

    // set initial values of the new environment
    //environment.rescue_probability = 4;
    environment.rain_probability = rain_prob * 15;
    environment.hunt_capability = option == 1 ? environment.next_scenario1.hunt_capability * 500 : environment.next_scenario2.hunt_capability * 500;
    environment.fish_availability = option == 1 ? environment.next_scenario1.fish_availability * 500 : environment.next_scenario2.fish_availability * 500;
    environment.wood_available = rand_lim(20) + 10;
    environment.temp = rand_lim(25) + 5;
    environment.minutes_since_last_rain = 24*60*3; // 3 days
    environment.minutes_rain_left = 0;
    environment.fire = 0;

    create_next_scenarios();
    show_status();
    return;
}

// this function is called by thread every gameplay hour
void handle_rain() {
    //old implementation:
    //int r = rand_lim(10);
    //if (minutes == m_increment && r > 2) return;
    //float f_days = environment.minutes_since_last_rain / 1440;
    //float f_rain = f_days - r * 1440;

    int r = rand_lim(100);
    float f_rain = r > (float) environment.rain_probability / 24 ? 0 : 1;
    // since handle_rain is called every gameplay hour
    // we have to divide by 24 the daily probability of raining

    // refill rain_catcher_fills, if is raining
    if (environment.minutes_since_last_rain < 0 && inventory.rain_catcher_fills < 3)
        inventory.rain_catcher_fills = 3;

    // stop raining
    if ( environment.minutes_rain_left < 0) {
        environment.minutes_rain_left = 0;
        environment.minutes_since_last_rain = 0;
    }

    // start raining
    // TODO mejorar este elseif agregando hidratación
    else if (environment.minutes_since_last_rain > 0
            && f_rain > 0
        )  {
        int r = rand_lim(2880); // 2 days max
        environment.minutes_rain_left = r;
        environment.minutes_since_last_rain = 0;

    // continue raining
    } else if (environment.minutes_rain_left) {
        environment.minutes_rain_left -= 60;
        environment.minutes_since_last_rain -= 60; // raining for x minutes !!

    //continue without rain
    } else {
        environment.minutes_since_last_rain += 60;
    }
    return;
}

void start_fire() {
    if (! goods.tinder) return;
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    forward_time(15);
    goods.tinder--;
    environment.fire = 20;
    show_status();
    return;
}

void fill_bottles_water_with_rain_catcher_fills() {
    if (inventory.empty_bottles && inventory.rain_catcher_fills) {
        inventory.rain_catcher_fills--;
        inventory.empty_bottles--;
        inventory.filled_safe_bottles++;
    }
    show_status();
    return;
}

void boil_water() {
    if ( ! inventory.filled_unsafe_bottles || ! environment.fire) return;
    inventory.filled_unsafe_bottles--;
    inventory.filled_safe_bottles++;
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    forward_time(15);
    show_status();
    return;
}

void feed_wood_to_fire() {
    if (! goods.wood) return;
    goods.wood--;
    environment.fire += environment.fire >= 120 ? 120 : 60;
    show_status();
    return;
}

void eat_beans_can() {
    if (! inventory.beans_can) return;
    if (inventory.beans_can && health.energy / 10.0 > 100) {
        msg("Energy is full. Shouldn't waste food!");
        return;
    }
    inventory.beans_can--;
    increase_energy(BEANS_CAN_ENERGY);
    show_status();
    return;
}

void increase_heat(float t) {
    health.heat += t;
    if (health.heat > 1000) health.heat = 1000;
}
void decrease_heat(float t) {
    health.heat -= t;
    if (health.heat < 0) health.heat = 0;
}

void increase_hydratation(float t) {
    health.hydratation += t;
    if (health.hydratation > 1000) health.hydratation = 1000;
}
void decrease_hydratation(float t) {
    health.hydratation -= t;
    if (health.hydratation < 0) health.hydratation = 0;
}

void increase_energy(float t) {
    health.energy += t;
    if (health.energy > 1000) health.energy = 1000;
}
void decrease_energy(float t) {
    health.energy -= t;
    if (health.energy < 0) health.energy = 0;
}

void increase_stamina(float t) {
    health.stamina += t;
    if (health.stamina > 1000) health.stamina = 1000;
}
void decrease_stamina(float t) {
    health.stamina -= t;
    if (health.stamina < 0) health.stamina = 0;
}


/* return a random number between 0 and limit inclusive. */
int rand_lim(int limit) {

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}

void * input(void * p) {
    while (! shall_quit) {
        int d = getch();

        if (d == ' ') {
            in_pause = ! in_pause;
            if (in_pause) {
                wmove(stdscr, 0, 0);
                wclrtobot(stdscr);
                mvprintw(0, 0, "PAUSE");
                refresh();
                continue;
            }
            show_status();
        } else if (d == 'q') {
            shall_quit = 1;
            msg("QUITTING !");
            wmove(stdscr, 0, 0);
            wclrtobot(stdscr);
            refresh();
            //sleep(1);
            pthread_cancel(forward_time_thread);
            pthread_cancel(rain_thread);
            return NULL;
        } else if (d == 'b') {
            boil_water();
        } else if (d == 'd') {
            get_wood();
        } else if (d == 'e') {
            feed_wood_to_fire();
        } else if (d == 'f') {
            start_fire();
        } else if (d == 'A') {
            travel(1);
        } else if (d == 'B') {
            travel(2);
        } else if (d == 'm') {
            in_travel_menu=1;
            show_moving_options();
        } else if (d == 'n') {
            navigate(2);
        } else if (d == 'o') {
            eat_beans_can();
        } else if (d == 'r') {
            rest(HOURS_TO_REST);
        } else if (d == 't') {
            forward_time(60);
            check_health();
            show_status();
        } else if (d == 'w') {
            drink_water(0);
        } else if (d == 'y') {
            fill_bottles_water_with_rain_catcher_fills();
        } else if (d != -1) {
            mvprintw(LINES-1, 0, "%d %c", d, d);
            wclrtoeol(stdscr);
            refresh();
        }
    }
}

int main(int argc, char * argv[]) {

    // set initial seed for random
    srand(time(NULL));

    // set initial values
    datetime.day = 0;
    datetime.minutes = 0;
    miles_travelled = 0;
    //miles_travelled = 120;
    health.heat = 1000;
    health.hydratation = 1000;
    health.energy = 1000;
    health.stamina = 1000;

    // set initial values of scenario
    // These will change when moving to other scenarios
    environment.rain_probability = 70;
    environment.hunt_capability = 0;
    environment.fish_availability = 0;
    //environment.rescue_probability = 4;
    environment.wood_available = 25;
    environment.temp = 21;
    environment.minutes_since_last_rain = 24*60*3; // 3 days
    environment.minutes_rain_left = 0;
    environment.fire = 0;

    create_next_scenarios();

    // some initial inventory and goods
    // goods.wood = 900;
    goods.wood = 0;
    // goods.tinder = 900;
    goods.tinder = 2;
    goods.rope = 0;
    goods.bait = 0;
    inventory.empty_bottles = 0;
    inventory.filled_safe_bottles = 2;
    // inventory.filled_safe_bottles = 990;
    inventory.filled_unsafe_bottles = 1;
    inventory.matches = 0;
    inventory.rain_catcher = 1;
    //inventory.beans_can = 60;
    inventory.beans_can = 2;

    initscr();
    noecho();
    cbreak();
    curs_set(FALSE);
    //wtimeout(stdscr, -1);
    //wtimeout(stdscr, s_loop * 1000);

    pthread_create (&forward_time_thread, NULL, time_thread_function, NULL);
    usleep(10000);
    pthread_create (&input_thread, NULL, input, NULL);
    usleep(10000);
    pthread_create (&rain_thread, NULL, rain_thread_function, NULL);

    pthread_join(forward_time_thread, NULL);
    pthread_join(input_thread, NULL);
    pthread_join(rain_thread, NULL);

    endwin();
}

