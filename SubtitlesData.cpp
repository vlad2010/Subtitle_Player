#include "SubtitlesData.h"
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

#include "Doc_Format.h"

using namespace std;

#define MODEL_LOG "model.txt"

bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

//to handle Win EOLs   CR/LF
void trim_left(std::string &line) 
{
    size_t endpos = line.find_last_not_of(" \t\n\r");
    Doc_Format(MODEL_LOG, "Endpos line: %d  size:%d line %s ", endpos, line.size(), line.c_str() );

    if( string::npos != endpos )
    {
        line = line.substr( 0, endpos+1 );
    }
    else
    {
        line.clear();
    }
}

SubtitlesData::SubtitlesData(char *file, int handler)
{
    std::ifstream infile(file);
    int local_index=INITIAL_INDEX;

    //the first subtitle is empty
    subtitles.push_back(subtitle_item());

    std::string line;
    while (std::getline(infile, line))
    {
        subtitle_item next_subtitle;

        trim_left(line);
        Doc_Format(MODEL_LOG, "\n\nAfter trim next line: %s ", line.c_str() );

        if(is_number(line))
        {
            next_subtitle.num = atoi( line.c_str() );

            std::string timing_line;
            std::getline(infile, timing_line);
            trim_left(timing_line);
            next_subtitle.timing_line = timing_line;

            getTimePairFromString(timing_line, next_subtitle.FromToTimes);

            std::string text_line;
            while (std::getline(infile, text_line))
            {
                trim_left(text_line);
                Doc_Format(MODEL_LOG, "next line length: %d text_line:%s "
                , text_line.size(), text_line.c_str() );

                if(text_line.size()==0)
                {
                	next_subtitle.index=local_index++;
                    subtitles.push_back(next_subtitle);
                	break;
                }
                next_subtitle.text_lines.push_back(text_line);
            }//while inner
        }//if
        else
        {
            Doc_Format(MODEL_LOG, "Line %s is not number ", line.c_str() );    
        }
    }//while

}//end ctor

int SubtitlesData::size() const
{
    return subtitles.size();
}

void SubtitlesData::print_subtitles() 
{
    for (std::vector<subtitle_item>::iterator it = subtitles.begin() ; it != subtitles.end(); ++it)
    {
        print_subtitle(*it);;
    }
}

void SubtitlesData::print_subtitle(subtitle_item &sub_item) const
{
    char *data = "DATA.txt";

    Doc_Format(data, "index:%d num:%d time:%s from:%d to:%d",
    sub_item.index, sub_item.num, sub_item.timing_line.c_str(),
    sub_item.FromToTimes.first, sub_item.FromToTimes.second);

    for (std::vector<std::string>::iterator it = sub_item.text_lines.begin() 
                  ; it != sub_item.text_lines.end(); ++it)
    {
        Doc_Format(data, "sub: %s ", (*it).c_str());
    }
    Doc_Format(data, " ");
}

void SubtitlesData::print_subtitle(int num) const
{
    subtitle_item sub_item = subtitles[num];
    print_subtitle(sub_item);
}

PlayerTime SubtitlesData::timeStringToMsec(std::string &time_string) const
{
    std::size_t found_comma = time_string.rfind(",");
    std::size_t first_semicolon = time_string.find(":");
    std::size_t last_semicolon =  time_string.rfind(":");

    if(first_semicolon==last_semicolon) //something wrong 
    {
        return 0;
    }

    if(first_semicolon==0 || last_semicolon==0 || found_comma==0) //something wrong 
    {
        return 0;
    }

    std::string hours = time_string.substr(0,first_semicolon);
    std::string mins = time_string.substr(first_semicolon+1, last_semicolon-first_semicolon-1);
    std::string secs = time_string.substr(last_semicolon+1, found_comma-last_semicolon-1);
    std::string msecs = time_string.substr(found_comma+1);

    return std::stoi(hours)*60*60*1000 + std::stoi(mins)*60*1000  + std::stoi(secs)*1000 + std::stoi(msecs); ///milliseconds
}


void SubtitlesData::getTimePairFromString(std::string time_string , std::pair<PlayerTime, PlayerTime> &times  ) const
{
    std::string delim = string(" --> ");
    std::size_t delimiter = time_string.find(" --> ");

    std::string time_from = time_string.substr(0,delimiter);
    std::string time_to = time_string.substr(delimiter+delim.size());

    times.first = timeStringToMsec(time_from);
    times.second = timeStringToMsec(time_to);
}

subtitle_item SubtitlesData::current_subtitle(PlayerTime time) const
{
    int sz = subtitles.size();
    for (int i=INITIAL_INDEX; i< sz ; i++)
    {
        subtitle_item sub = subtitles[i];
        subtitle_item next_sub;

        if(i<(sz-1))
        {
            next_sub = subtitles[i+1];
        }

        if(sub.FromToTimes.first > time)
            return sub; 


        if(sub.FromToTimes.first <=time && sub.FromToTimes.second >=time )
        {
            return sub;
        }

        if(sub.FromToTimes.second <=time && next_sub.FromToTimes.first >=time )
        {
            return next_sub;
        }

    }//for
    //return empty subtitle
    return subtitles[0];
}

subtitle_item SubtitlesData::subtitle_with_index(int index) const
{
    return subtitles[index];
}

PlayerTime SubtitlesData::maximum_Time() const
{
    subtitle_item last = subtitles.back();
    return last.FromToTimes.second;
}
