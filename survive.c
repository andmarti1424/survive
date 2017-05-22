// includes
#include <ncurses.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

// defines
#define m_increment 5       // gameplay minutes that increments in each loop
#define s_loop 5            // realtime seconds in loop
#define BEANS_CAN_ENERGY 300
#define REST_HOURS       4

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

short miles; //miles travelled

struct environment {
    short temp;
    short wood_available;
    float rain_probability; // probability of rain in a day (0 to 100)
    short minutes_rain_left;
    short minutes_since_last_rain;
    short fire; // 0 means no fire. different to 0 means fire left in minutes
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

/* *****************************************************************************
   FUNCIONES DE VISTA
   *****************************************************************************/

void print_main_menu() {
//    mvprintw(19, 0, "Inventory [1]");
//    wclrtoeol(stdscr);
    mvprintw(20, 0, "Get Wood                                [d]   (1 hour) ");
    wclrtoeol(stdscr);
    mvprintw(21, 0, "Start fire                              [f]   (15 min)");
    wclrtoeol(stdscr);
    mvprintw(22, 0, "Feed wood to fire                       [e]");
    wclrtoeol(stdscr);

    mvprintw(24, 0, "Drink water                             [w]");
    wclrtoeol(stdscr);
    mvprintw(25, 0, "Fill empty bottles with rain catcher    [y]");
    wclrtoeol(stdscr);
    mvprintw(26, 0, "Boil water                              [b]   (15 min)");
    wclrtoeol(stdscr);

    mvprintw(28, 0, "Eat beans can                           [o]");
    wclrtoeol(stdscr);
    mvprintw(30, 0, "Rest                                    [r]   (4 hours)");
    wclrtoeol(stdscr);
    mvprintw(31, 0, "Navigate                                [n]");
    wclrtoeol(stdscr);
    mvprintw(32, 0, "Shelter                                 [s]");
    wclrtoeol(stdscr);
    //mvprintw(33, 0, "Food                                    [o]");
    //wclrtoeol(stdscr);
    mvprintw(33, 0, "Crafting                                [c]");
    wclrtoeol(stdscr);
    mvprintw(34, 0, "Test (forward time)                     [t]");
    wclrtoeol(stdscr);
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
    //mvprintw(0, 0, "Day: %d", datetime.day);
    mvprintw(0, 0, "Day: %d                                  hour: %d         minutes: %d", datetime.day, datetime.minutes/60, datetime.minutes);
    wclrtoeol(stdscr);

    if (datetime.minutes >= 20*60) // > 20hs
        mvprintw(1, 0, "Hours darkness remaining: %d", (24+8)-datetime.minutes/60);
    else if (datetime.minutes < 8*60)
        mvprintw(1, 0, "Hours darkness remaining: %d", 8-datetime.minutes/60);
    else
        mvprintw(1, 0, "Hours daylight remaining: %d", 20-datetime.minutes/60);
    wclrtoeol(stdscr);

    mvprintw(2, 0, "temp: %d", environment.temp);
    wclrtoeol(stdscr);
//    mvprintw(3, 0, "miles travelled: %d", miles);
//    wclrtoeol(stdscr);

    if (environment.minutes_since_last_rain < 0 && environment.minutes_since_last_rain > -60)
        mvprintw(4, 0, "raining for %d minutes", -environment.minutes_since_last_rain);
    else if (environment.minutes_since_last_rain < 0)
        mvprintw(4, 0, "raining for %d hours", -environment.minutes_since_last_rain / 60);
    wclrtoeol(stdscr);

    if (environment.fire && environment.fire < 60)
        mvprintw(5, 0, "we have fire. minutes left: %d", environment.fire);
    else if (environment.fire && environment.fire > 60)
        mvprintw(5, 0, "we have fire. hours left: %d", environment.fire/60);
    else
        move(5, 0);
    wclrtoeol(stdscr);
    return;
}

void print_health() {
    mvprintw(7, 0, "heat: %.0f", health.heat / 10);
    wclrtoeol(stdscr);
    mvprintw(8, 0, "hydratation: %.0f", health.hydratation / 10);
    wclrtoeol(stdscr);
    mvprintw(9, 0, "energy: %.0f", health.energy / 10);
    wclrtoeol(stdscr);
    mvprintw(10, 0, "stamina: %.0f", health.stamina / 10);
    wclrtoeol(stdscr);
}

void print_goods() {
    mvprintw(18, COLS-30, "tinder: %d", goods.tinder);
    wclrtoeol(stdscr);
    mvprintw(19, COLS-30, "wood: %d", goods.wood);
    wclrtoeol(stdscr);
    mvprintw(20, COLS-30, "rope: %d", goods.rope);
    wclrtoeol(stdscr);
    mvprintw(21, COLS-30, "bait: %d", goods.bait);
    wclrtoeol(stdscr);

}
void print_inventory() {
    mvprintw(10, COLS-30, "Inventory:");
    wclrtoeol(stdscr);
    mvprintw(11, COLS-30, "empty bottles: %d", inventory.empty_bottles);
    wclrtoeol(stdscr);
    mvprintw(12, COLS-30, "filled safe bottles: %d", inventory.filled_safe_bottles);
    wclrtoeol(stdscr);
    mvprintw(13, COLS-30, "filled unsafe bottles: %d", inventory.filled_unsafe_bottles);
    wclrtoeol(stdscr);
    mvprintw(14, COLS-30, "matches: %d", inventory.matches);
    wclrtoeol(stdscr);
    mvprintw(15, COLS-30, "rain catcher: %d", inventory.rain_catcher);
    wclrtoeol(stdscr);
    mvprintw(16, COLS-30, "rain catcher fills: %d", inventory.rain_catcher_fills);
    wclrtoeol(stdscr);
    mvprintw(17, COLS-30, "beans can: %d", inventory.beans_can);
    wclrtoeol(stdscr);
}

void show_status() {
    print_day_hour_miles_temp();
    print_health();
    print_main_menu();
    print_goods();
    print_inventory();
    refresh();
}

void msg(char * s) {
    mvprintw(LINES - 1, 0, "%s", s);
    wclrtoeol(stdscr);
    refresh();
}
/* *****************************************************************************
   FIN DE FUNCIONES DE VISTA
   *****************************************************************************/


void check_health() {
    // TODO
    //if (! health.stamina == 0)
    //    force_sleep();

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
        getch();
        shall_quit = 1;
        //pthread_cancel(forward_time_thread);
    }
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
    if (! environment.fire && datetime.minutes >= 20*60 && datetime.minutes < 8*60) j = (float) rand_lim(2);
    else if (! environment.fire && (datetime.minutes >= 20*60 || datetime.minutes < 8*60)) j = (float) rand_lim(1);
    decrease_heat(j * 100);
    j = 0;

    // increase temp
    if (environment.fire && (datetime.minutes >= 20*60 || datetime.minutes < 8*60)) j = (float) rand_lim(1);
    else if (environment.fire && datetime.minutes >= 20*60 && datetime.minutes < 8*60) j = (float) (rand_lim(2) + 1);
    increase_heat(j * 100);

    // bajo energia
    decrease_energy(1000.0 / 21600 * minutes ); // 15 days without food

    // bajo hidratacion
    decrease_hydratation(1000.0 / 2160 * minutes); // 1.5 days without water

    // bajo stamina
    decrease_stamina(1000.0 / 1440 * minutes); // 24 hs max without rest

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
    }
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
        } else if (d == 'o') {
            eat_beans_can();
        } else if (d == 'r') {
            rest(REST_HOURS);
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

    environment.temp = 21;
    environment.rain_probability = 70;
    miles = 0;
    environment.wood_available = 25;
    environment.minutes_since_last_rain = 24*60*3; // 3 days
    environment.minutes_rain_left = 0;

    health.heat = 1000;
    health.hydratation = 1000;
    health.energy = 1000;
    health.stamina = 1000;

    goods.wood = 10;
    goods.tinder = 10;
    goods.rope = 0;
    goods.bait = 0;
    environment.fire = 0;

    inventory.empty_bottles = 0;
    inventory.filled_safe_bottles = 2;
    inventory.filled_unsafe_bottles = 1;
    inventory.matches = 0;
    inventory.rain_catcher = 1;
    inventory.beans_can = 10;

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





void rest(short hours) {
    pthread_create (&progress_bar_thread, NULL, show_progressbar, NULL);
    pthread_join(progress_bar_thread, NULL);
    forward_time(60 * hours);
    increase_stamina(1000.0 / 8 * hours); // 8 hours daily of rest?
    show_status();
    return;
}



// type 0 - we drink filled_bottles
void drink_water(short type) {
    if (type == 0 && inventory.filled_safe_bottles && health.hydratation == 1000)
        msg("Not thirsty. Shouldnt waste water!");
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
