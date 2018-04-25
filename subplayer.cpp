/////////////////////////////////////////////////
//
// Subtitle Player
// Created Thu Apr 01 2016
// Compile with gcc 4.8.4 on Ubuntu 14.04
//
/////////////////////////////////////////////////

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
#include <iostream>
#include <curses.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <mutex>

#include "Doc_Format.h"
#include "SubtitlesData.h"

/*
=============================================================================

              DEFINITIONS

=============================================================================
*/

#define SUBTITLE_TEXT_START_LINE 5

#define HELP_TEXT_LINE 15


#define TIME_SHIFT 60   //forward  backward  shift in secs
#define STATE_LOG "state.txt"
#define trace Doc_Format(STATE_LOG, "Line: %d File: %s ", __LINE__, __FILE__ );
#define TIMING_LINE 9

/*
=============================================================================

              GLOBAL VARIABLES

=============================================================================
*/
enum Timer_Types { SIGVAL_1HZ=1001, 
                   SIGVAL_SHOW_SUBTITLE, 
                   SIGVAL_HIDE_SUBTITLE 
                 };

SubtitlesData *pl; // //player storage

static int sec=0; //player time in seconds
static bool isPlayerPlaying = false; 
static int subtitle_index = INITIAL_INDEX;
static int maximum_time = 0;  
static std::mutex time_mutex;   //handle access to index and sec variables

timer_t t_id_hide;   //hide subtitle timer
timer_t t_id_show;   //show subtitle timer 
timer_t t_clock_id;  //1 Hz timer

/*
=============================================================================

              FUNCTION PROTOTYPES

=============================================================================
*/

void printTime(int sec_, int max_sec_);
//void printTimeAt(int sec, int x, int y);
void t_create(timer_t &t_id, sigevent &se, void (*f)(sigval val) , const int sigvalue);
void t_start(timer_t id,PlayerTime p_player, int periodic);
void t_stop(timer_t &t_id);
void erase_current_subtitle();
PlayerTime current_time_ms();
void trace_player_state();
void timer_get_msec(timer_t &t_id, PlayerTime &msec, PlayerTime &msec_iter );
void handle_forward_button();
void handle_back_button();
std::string getFormattedTimeString(int sec_);
void timeSynchroFromModel(PlayerTime t);

/*
=============================================================================

              IMPLEMENTATION

=============================================================================
*/
void trace_player_state()
{
    subtitle_item sub = pl->subtitle_with_index(subtitle_index);
    PlayerTime msec, msec_iter;
    timer_get_msec(t_id_show, msec, msec_iter );

    Doc_Format(STATE_LOG, "stt index:%d  sec:%d sub_index:%d  timeLine:%s next time msec:%d time msec_iter:%d "
    ,subtitle_index, sec, sub.index, (char*)sub.timing_line.c_str() , msec, msec_iter );
}


void timer_get_msec(timer_t &t_id, PlayerTime &msec, PlayerTime &msec_iter )
{
    struct itimerspec cur;
    timer_gettime(t_id, &cur);

    msec =  cur.it_value.tv_sec*1000+cur.it_value.tv_sec/1000000;
    msec_iter =  cur.it_interval.tv_sec*1000+cur.it_interval.tv_sec/1000000;
}


void refresh_play_process()
{
    PlayerTime ct = sec*PLAYER_SEC;
    subtitle_item sub_item = pl->current_subtitle(ct); 

    Doc_Format(STATE_LOG, "reshres_play() time:%d  time first:%d  index:%d",
    ct, sub_item.FromToTimes.first, sub_item.index);

    trace_player_state();

    time_mutex.lock();
    subtitle_index=sub_item.index;
    time_mutex.unlock();

    if(sub_item.FromToTimes.first > ct)
    {
        //  setup timer
        t_start(t_id_show, sub_item.FromToTimes.first-ct, 0); 
    } 
    else
    {
        // show subtitle
        int outline = SUBTITLE_TEXT_START_LINE;
        for (std::vector<std::string>::iterator it = sub_item.text_lines.begin() ; it != sub_item.text_lines.end(); ++it)
        {
            mvprintw(outline++, 0, "%s", (*it).c_str() );
        }
        refresh(); //nsurses

        t_start(t_id_hide, sub_item.FromToTimes.second-sub_item.FromToTimes.first, 0);
    }  

}

void timeSynchroFromModel(PlayerTime t)
{
    time_mutex.lock();
    sec=t/PLAYER_SEC;
    time_mutex.unlock();

    printTime(sec, maximum_time);
    refresh();
}

//handle timer events
void timer_show_handle(sigval val)
{
    Doc_Format(STATE_LOG, "show timer handle sigval %d ", val);
    subtitle_item cur_sub_item = pl->subtitle_with_index(subtitle_index);

    //time syncro with current subtitle 
    if(cur_sub_item.FromToTimes.first < (sec*PLAYER_SEC) )
    {
        timeSynchroFromModel(cur_sub_item.FromToTimes.first);
    }

    int outline = SUBTITLE_TEXT_START_LINE;
    for (std::vector<std::string>::iterator it = cur_sub_item.text_lines.begin() ; it != cur_sub_item.text_lines.end(); ++it)
    {
        mvprintw(outline++, 0, "%s", (*it).c_str() );
    }

    mvprintw(9, 0, "%s", cur_sub_item.timing_line.c_str() );
    refresh();
    t_start(t_id_hide, cur_sub_item.FromToTimes.second-cur_sub_item.FromToTimes.first, 0);
}

//handle all timer events
void timer_hide_handle(sigval val)
{
    Doc_Format(STATE_LOG, "hide timer handle sigval %d ", val);
    subtitle_item cur_sub_item = pl->subtitle_with_index(subtitle_index);  
    erase_current_subtitle();

    //time syncro with current subtitle 
    if(cur_sub_item.FromToTimes.second > (sec*PLAYER_SEC) )
    {
        timeSynchroFromModel(cur_sub_item.FromToTimes.second);
    }

    int sz = pl->size();

    if(subtitle_index==(sz-1))
    {   
        t_start(t_clock_id, 0, 0); //stop 
        return;
    }
    else
    {
        time_mutex.lock();
        subtitle_index++;
        time_mutex.unlock();
    }

    subtitle_item next_sub_item = pl->subtitle_with_index(subtitle_index);

    if(isPlayerPlaying)
        t_start(t_id_show,  next_sub_item.FromToTimes.first - cur_sub_item.FromToTimes.second, 0);
}


//1 hz counter 
void timer_clock_handle(sigval val)
{
  Doc_Format(STATE_LOG, "1hz timer handle sigval %d ", val);
  time_mutex.lock();
  sec++;
  time_mutex.unlock();

  printTime(sec, maximum_time);
  trace_player_state();
  refresh();
}

void printTime(int sec, int maxsec)
{
  mvprintw( 2,0 , "Time: %s/%s",
     getFormattedTimeString(sec).c_str(), 
     getFormattedTimeString(maxsec).c_str() );
}

std::string getFormattedTimeString(int sec_)
{
    char buf[32];

    int h = sec_/(60*60);
    int m = (sec_-(h*60*60)) /(60);
    int s = sec_-(h*60*60)  -(m*60);
    sprintf(buf, "%02d:%02d:%02d", h, m ,s);

    return std::string(buf);
}

void t_create(timer_t &t_id, sigevent &se, void (*f)(sigval val) , const int sigvalue  )
{
    se.sigev_notify = SIGEV_THREAD;
    se.sigev_value.sival_int = sigvalue;
    se.sigev_notify_function = f;
    se.sigev_notify_attributes = 0;
    if (-1 == timer_create(CLOCK_REALTIME, &se, &t_id))
    {
        std::cerr << "timer_create failed : " << errno << std::endl; 
    }
}

void t_start(timer_t id,PlayerTime p_player, int periodic)
{
    struct itimerspec its;

    int secs =  p_player/PLAYER_SEC; 
    int nanos = (p_player - (secs*PLAYER_SEC))*1000000;
    its.it_value.tv_sec = secs;
    its.it_value.tv_nsec = nanos;

    if(periodic)
    {
        its.it_interval.tv_sec = its.it_value.tv_sec;
        its.it_interval.tv_nsec = its.it_value.tv_nsec;
    }
    else
    {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }

    timer_settime(id, 0, &its, NULL);
}

void t_stop(timer_t &t_id)
{
    t_start(t_id,0, 0);
}

//app deinitialize
void deinitialize(WINDOW* mainwin)
{
    delwin(mainwin);
    endwin();
    refresh();

    if(pl)
        free(pl);
}

//app usage
void print_usage(char *exe_name)
{
    std::cout << "Usage: " << exe_name << " sub_title_file_name" << std::endl;
}

// remove current subtitle from screen
void erase_current_subtitle()
{
    subtitle_item cur_sub_item = pl->subtitle_with_index(subtitle_index);  

    for(int i=0; i < cur_sub_item.text_lines.size(); i++)
    {
        move(SUBTITLE_TEXT_START_LINE+i, 0);
        clrtoeol();
    }    

    move(TIMING_LINE, 0);
    clrtoeol();
    refresh();
}


PlayerTime current_time_ms()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    PlayerTime ms = tp.tv_sec * PLAYER_SEC + tp.tv_usec / PLAYER_SEC;
    return ms;
}

//forward button pressed
void handle_forward_button()
{
    erase_current_subtitle();
    time_mutex.lock();
    bool stop_playing = (sec+TIME_SHIFT > pl->maximum_Time()/PLAYER_SEC);

    if(stop_playing)
    {
        sec=pl->maximum_Time()/PLAYER_SEC;
        t_stop(t_clock_id);
        isPlayerPlaying= false;
    }
    else
    {
        sec=sec+TIME_SHIFT;
    }
    time_mutex.unlock();
    t_stop(t_id_show); t_stop(t_id_hide);

    if(!stop_playing)
        refresh_play_process();

    printTime(sec, maximum_time);
}

//back button pressed
void handle_back_button()
{
    erase_current_subtitle();
    time_mutex.lock();
    if(sec-TIME_SHIFT < 0 )
    {  
      sec=0;
      subtitle_index=INITIAL_INDEX;
    }
    else
    {
      sec=sec-TIME_SHIFT;
    }

    time_mutex.unlock();
    t_start(t_clock_id, PLAYER_SEC, 1);
    isPlayerPlaying= true;

    t_stop(t_id_show); t_stop(t_id_hide);
    refresh_play_process();
    printTime(sec, maximum_time);
}



/*
=============================================================================

              MAIN

=============================================================================
*/
int main(int argc, char* argv[])
{
    if(argc<2)
    {
        std::cout << "Error: No subtitle file name provided." << std::endl;
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    char * subtitle_file_name = argv[1];
    isPlayerPlaying= true;

    if(access( subtitle_file_name, R_OK ) == -1)
    {
        std::cout << "Error: Provided srt file " << argv[1] << " not exist or not readable." 
        << std::endl;
        return EXIT_FAILURE;
    }

    WINDOW * mainwin;
    int ch;

    //  Initialize ncurses  
    if ( (mainwin = initscr()) == NULL ) 
    {
        fprintf(stderr, "Error initializing ncurses.\n");
        exit(EXIT_FAILURE);
    }

    //timer show
    sigevent se_show;
    t_create(t_id_show, se_show, timer_show_handle,SIGVAL_SHOW_SUBTITLE );

    //timer hide
    sigevent se_hide;
    t_create(t_id_hide, se_hide, timer_hide_handle, SIGVAL_HIDE_SUBTITLE);

    //1Hz timer 
    sigevent se_1hz;
    t_create(t_clock_id, se_1hz, timer_clock_handle, SIGVAL_1HZ);

    //initialize mutex

    printTime(sec, maximum_time);
    t_start(t_clock_id, PLAYER_SEC, 1);

    try
    {
        pl = new SubtitlesData(subtitle_file_name,0);
        pl->print_subtitles(); // DEBUG dump all titles to file  

        if(pl->size()<=INITIAL_INDEX)
        {
            deinitialize(mainwin);
            std::cerr << "Can't parse subtitles file. size:" << pl->size() <<  std::endl;
            return EXIT_FAILURE;
        }

        maximum_time = pl->maximum_Time()/PLAYER_SEC;
        printTime(sec,maximum_time );

        Doc_Format( STATE_LOG,  "vector size is %d ", pl->size() );
        subtitle_item first_subtitle = pl->subtitle_with_index(INITIAL_INDEX);
        mvprintw(HELP_TEXT_LINE, 0, "Q - QUIT, F - FORWARD 1 MINUTE, B - BACK 1 MINUTE   File: %s", subtitle_file_name);
        refresh();

        //timer create for first subtitle
        t_start(t_id_show,first_subtitle.FromToTimes.first, 0);
        while ( (ch = getch()) != 'q' && ch !='Q'  ) 
        {
            if((ch=='f') || (ch=='F'))  //forward button
            {
                handle_forward_button();
            }

            if((ch=='b') || (ch=='B'))  //back button
            {
                handle_back_button();
            }
        }//while

    }//try
    catch(...)
    {
        //handle it somehow
        std::cerr << "Sorry. Something went wrong." <<  std::endl;
    }

    deinitialize(mainwin);
    return EXIT_SUCCESS;
}

