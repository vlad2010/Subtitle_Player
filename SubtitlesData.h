#include <string>
#include <vector>

typedef unsigned long long int PlayerTime;

#define PLAYER_SEC 1000
#define INITIAL_INDEX 1

/*
=============================================================================

              SUBTITLE TYPE

=============================================================================
*/
struct subtitle_item 
{
	unsigned int index;
	unsigned int num;

	std::pair<PlayerTime, PlayerTime> FromToTimes;
	std::string timing_line;
	std::vector<std::string> text_lines;

	subtitle_item():index(0), num(0) {  }
};

/*
=============================================================================

              SUBTITLE DATA CLASS DECLATION

=============================================================================
*/
class SubtitlesData
{
public:
	SubtitlesData(char *file, int handler);

	//return current subtitle or closest from greater timer
	subtitle_item current_subtitle(PlayerTime ) const;
	subtitle_item subtitle_with_index(int index) const;

	int size() const;//how many subtitles we have
	PlayerTime maximum_Time() const; //end time of last subtitle

	void print_subtitles() ;

private:
	// subtitle storage
	std::vector<subtitle_item> subtitles;

	//service functions
	PlayerTime timeStringToMsec(std::string &time_string) const;
	void getTimePairFromString(std::string time_string , std::pair<PlayerTime, PlayerTime> &times  ) const;

	//debug 

	void print_subtitle(int num) const;
	void print_subtitle(subtitle_item &sub_item) const;
};