// ___ ==================================================
// ___ file: main.cpp
// ___ project: Sudoku App
// ___ author: Paulina Kalicka
// ___ ==================================================

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_image.h>

#include <fstream>
#include <string>
#include <vector>
#include <future>

#define COLOR_BACKGROUND al_map_rgb(232,232,232)
#define COLOR_HOVERED al_map_rgba(232,232,0,132)
#define COLOR_ACTIVED al_map_rgba(132,132,0,132)
#define COLOR_DARK al_map_rgb(13,13,13)
#define COLOR_REGION1 al_map_rgb(0,0,132)
#define COLOR_REGION2 al_map_rgb(0,132,132)
#define COLOR_SAME_NUMBER al_map_rgba(0,255,0,232)
#define COLOR_DUPLICATE_NUMBER al_map_rgba(255,0,0,232)

const int SIZE = 9;
const int MIN_CLUES = 17;
const int NUM_CLUES = 20;
const int MAX_FILE_NAME = 20;
const float DISPLAY_W = 1000.f;
const float DISPLAY_H = 1000.f;
const double MAX_THREAD_TIME = 3.0;

// ___ < VARIABLES ASSOCIATED WITH GENERATING SUDOKU
volatile bool loading = 0;
volatile bool failed = 0;
volatile bool done = 0;
volatile double time_started_thread = 0.0;
// ___ > VARIABLES ASSOCIATED WITH GENERATING SUDOKU

enum BUTTON
{
	SOLVE, RESET, GENERATE, SAVE, LOAD,
	NUMBER_BUTTONS
};
enum ACTIVE_AREA
{
	GRID = 1, INPUT
};

struct Box
{
	int number = 0;
	int x = 0;
	int y = 0;
	int region = 0;
	int hovered = 0;
	float px1 = 0.f;
	float py1 = 0.f;
	float px2 = 0.f;
	float py2 = 0.f;
	float cx = 0.f;
	float cy = 0.f;
};
struct Grid
{
	float px1 = 0.f;
	float py1 = 0.f;
	float px2 = 0.f;
	float py2 = 0.f;
};
struct Button
{
	float px1 = 0.f;
	float py1 = 0.f;
	float px2 = 0.f;
	float py2 = 0.f;
	float cx = 0.f;
	float cy = 0.f;
	float text_w = 0.f;
	int hovered = 0;
	const char* text = "";
};
struct InputBox
{
	int active = 0;
	int hovered = 0;
	std::string text = "";
};

void DrawText(ALLEGRO_FONT* font, float width, float height, float pos_x, float pos_y,
			  int centre, ALLEGRO_COLOR color, const char* text)
{
	float text_width = al_get_text_width(font, text);
	float text_height = al_get_font_line_height(font);
	float scalex = width / text_width;
	float scaley = height / text_height;
	float scale = scalex < scaley ? scalex : scaley;

	ALLEGRO_TRANSFORM prev_t;
	ALLEGRO_TRANSFORM t;

	al_copy_transform(&prev_t, al_get_current_transform());
	al_identity_transform(&t);
	al_translate_transform(&t, -pos_x, -pos_y);
	al_scale_transform(&t, scale, scale);
	al_translate_transform(&t, pos_x, pos_y);
	al_compose_transform(&t, &prev_t);
	al_use_transform(&t);
	al_draw_textf(font, color, pos_x, centre ? pos_y - (text_height / 2) : pos_y, centre, "%s", text);
	al_use_transform(&prev_t);
}
void TransformDisplay(ALLEGRO_DISPLAY* display)
{
	al_acknowledge_resize(display);

	ALLEGRO_TRANSFORM t;

	float scaleX = al_get_display_width(display) / DISPLAY_W;
	float scaleY = al_get_display_height(display) / DISPLAY_H;
	float scale = scaleX < scaleY ? scaleX : scaleY;

	al_identity_transform(&t);
	al_scale_transform(&t, scale, scale);
	if(scale == scaleX)
	{
		float y = (al_get_display_height(display) / 2) - (DISPLAY_H * scale * 0.5f);
		al_translate_transform(&t, 0, y);
	}
	else if(scale == scaleY)
	{
		float x = (al_get_display_width(display) / 2) - (DISPLAY_W * scale * 0.5f);
		al_translate_transform(&t, x, 0);
	}

	al_use_transform(&t);
}
void TransformMouse(float* mx, float* my)
{
	ALLEGRO_TRANSFORM t;

	al_copy_transform(&t, al_get_current_transform());
	al_invert_transform(&t);
	al_transform_coordinates(&t, mx, my);
}

void UpdateBoxes(Box boxes[SIZE][SIZE], int sudoku[SIZE][SIZE])
{
	for(int y = 0; y < SIZE; ++y)
		for(int x = 0; x < SIZE; ++x)
			boxes[x][y].number = sudoku[x][y];
}
void FillBoxes(Box boxes[SIZE][SIZE], int number)
{
	for(int y = 0; y < SIZE; ++y)
		for(int x = 0; x < SIZE; ++x)
			boxes[x][y].number = number;
}

// ___ < FUNCTIONS FOR SUDOKU
void UpdateSudoku(int sudoku[SIZE][SIZE], Box boxes[SIZE][SIZE])
{
	for(int y = 0; y < SIZE; ++y)
		for(int x = 0; x < SIZE; ++x)
			sudoku[x][y] = boxes[x][y].number;
}
void FillSudoku(int sudoku[SIZE][SIZE], int number)
{
	for(int y = 0; y < SIZE; ++y)
		for(int x = 0; x < SIZE; ++x)
			sudoku[x][y] = number;
}
bool IsInRow(int sudoku[SIZE][SIZE], int row, int number)
{
	for(int col = 0; col < SIZE; ++col)
		if(sudoku[row][col] == number)
			return 1;
	return 0;
}
bool IsInColumn(int sudoku[SIZE][SIZE], int col, int number)
{
	for(int row = 0; row < SIZE; ++row)
		if(sudoku[row][col] == number)
			return 1;
	return 0;
}
bool IsInGrid(int sudoku[SIZE][SIZE], int begin_row, int begin_col, int number)
{
	for(int row = 0; row < 3; ++row)
		for(int col = 0; col < 3; ++col)
			if(sudoku[row + begin_row][col + begin_col] == number)
				return 1;
	return 0;
}
bool IsUnique(int sudoku[SIZE][SIZE], int row, int col, int number)
{
	if(!IsInRow(sudoku, row, number))
		if(!IsInColumn(sudoku, col, number))
			if(!IsInGrid(sudoku, row - (row % 3), col - (col % 3), number))
				if(sudoku[row][col] == 0)
					return 1;
	return 0;
}
bool IsSolved(int sudoku[SIZE][SIZE])
{
	/*
		check if in each row, column and small grid
		there is only one box with the same number
		false: sudoku was not solved correctly
		true: sudoku was solved correctly
	*/

	for(int row = 0; row < SIZE; ++row)
	{
		for(int col = 0; col < SIZE; ++col)
		{
			int num_in_row = 0;
			for(int col1 = 0; col1 < SIZE; ++col1)
				if(sudoku[row][col1] == sudoku[row][col])
					num_in_row++;
			if(num_in_row != 1)return 0;

			int num_in_col = 0;
			for(int row1 = 0; row1 < SIZE; ++row1)
				if(sudoku[row1][col] == sudoku[row][col])
					num_in_col++;
			if(num_in_col != 1)return 0;

			int num_in_grid = 0;
			for(int row1 = 0; row1 < 3; ++row1)
				for(int col1 = 0; col1 < 3; ++col1)
					if(sudoku[row1 + (row - (row % 3))][col1 + (col - (col % 3))] == sudoku[row][col])
						num_in_grid++;
			if(num_in_grid != 1)return 0;
		}
	}
	return 1;
}
bool CanSolve(int sudoku[SIZE][SIZE])
{
	/*
		check if in each row, column and small grid
		there is not more than one box with the same number
		false: in this case solving will last really long and there will not be a solution
		true: in this case solving makes sense
	*/

	for(int row = 0; row < SIZE; ++row)
	{
		for(int col = 0; col < SIZE; ++col)
		{
			int num_in_row = 0;
			for(int col1 = 0; col1 < SIZE; ++col1)
				if(sudoku[row][col1] != 0)
					if(sudoku[row][col1] == sudoku[row][col])
						num_in_row++;
			if(num_in_row > 1)return 0;

			int num_in_col = 0;
			for(int row1 = 0; row1 < SIZE; ++row1)
				if(sudoku[row1][col] != 0)
					if(sudoku[row1][col] == sudoku[row][col])
						num_in_col++;
			if(num_in_col > 1)return 0;

			int num_in_grid = 0;
			for(int row1 = 0; row1 < 3; ++row1)
				for(int col1 = 0; col1 < 3; ++col1)
					if(sudoku[row1 + (row - (row % 3))][col1 + (col - (col % 3))] != 0)
						if(sudoku[row1 + (row - (row % 3))][col1 + (col - (col % 3))] == sudoku[row][col])
							num_in_grid++;
			if(num_in_grid > 1)return 0;
		}
	}
	return 1;
}
bool Get0Location(int sudoku[SIZE][SIZE], int& row, int& col)
{
	/*
		check if there is 0 box in given sudoku
		and change values passed by reference at the same time
		false: sudoku is probably solved
		true: sudoku is not filled yet
	*/

	for(row = 0; row < SIZE; ++row)
		for(col = 0; col < SIZE; ++col)
			if(sudoku[row][col] == 0)
				return 1;
	return 0;
}
bool SolveSudoku(int sudoku[SIZE][SIZE])
{
	int row;
	int col;

	if(!Get0Location(sudoku, row, col))
		return 1; // there is no more boxes to fill

	for(int number = 1; number <= SIZE; ++number)
	{
		if(IsUnique(sudoku, row, col, number))
		{
			sudoku[row][col] = number;

			if(al_get_time() - time_started_thread > MAX_THREAD_TIME)
			{
				// in case solving will last too long
				// btw - SolveSudoku function does not run int another async function
				failed = 1;
				return 0;
			}

			// it leads to correct solution?
			if(SolveSudoku(sudoku))
				return 1;

			// so not... that attempt was not good - try again ;)
			sudoku[row][col] = 0;
		}
	}
	return 0;
}
bool SaveSudoku(int sudoku[SIZE][SIZE], std::string name)
{
	std::string file_name = name + ".txt";
	std::ofstream file;

	file.open("saved_sudoku//" + file_name);
	if(file.is_open())
	{
		for(int x = 0; x < SIZE; ++x)
		{
			for(int y = 0; y < SIZE; ++y)
				file << sudoku[x][y] << " ";
			file << "\n";
		}

		file.close();
		return 1;
	}
	return 0;
}
bool LoadSudoku(int sudoku[SIZE][SIZE])
{
	int loaded = 1;

	ALLEGRO_FILECHOOSER* filechooser = al_create_native_file_dialog("C:", "Choose a .txt file", "*.*;*.txt;", 1);
	if(!filechooser)loaded = 0;

	if(loaded)
	{
		al_show_native_file_dialog(0, filechooser);

		int n = al_get_native_file_dialog_count(filechooser);
		if(n == 1)
		{
			const char* path = al_get_native_file_dialog_path(filechooser, 0);
			std::ifstream file;

			file.open(path);
			if(file.is_open())
			{
				for(int x = 0; x < SIZE; ++x)
					for(int y = 0; y < SIZE; ++y)
						file >> sudoku[x][y];
			}
			else loaded = 0;
		}
		else loaded = 0;
	}

	al_destroy_native_file_dialog(filechooser);
	return loaded;
}
bool GenerateSudoku(int sudoku[SIZE][SIZE], int clues)
{
	/*
		try to generate a valid sudoku
		and make sure it can be solved
		terminate and return false if exceed the max time
	*/

	time_started_thread = al_get_time();
	loading = 1;
	failed = 0;

	bool end = 0;
	do
	{
		int num_clues = clues;
		int final_sudoku[9][9] = {0};
		int active_position[SIZE][SIZE];

		std::vector<int>posibilities[SIZE][SIZE];

		for(int x = 0; x < SIZE; ++x)
		{
			for(int y = 0; y < SIZE; ++y)
			{
				for(int i = 1; i <= SIZE; ++i)
					posibilities[x][y].push_back(i);
				active_position[x][y] = 1;
			}
		}

		while(num_clues > 0)
		{
			int rand_x, rand_y;

			do
			{
				rand_x = rand() % SIZE;
				rand_y = rand() % SIZE;
			}
			while(!(active_position[rand_x][rand_y]));

			int rand_i = rand() % (posibilities[rand_x][rand_y].size());
			int rand_num = posibilities[rand_x][rand_y][rand_i];

			if(final_sudoku[rand_x][rand_y] == 0)
			{
				if(IsUnique(final_sudoku, rand_x, rand_y, rand_num))
				{
					final_sudoku[rand_x][rand_y] = rand_num;
					--num_clues;
					active_position[rand_x][rand_y] = 0;
					posibilities[rand_x][rand_y].erase(posibilities[rand_x][rand_y].begin() + rand_i);
				}
				if(posibilities[rand_x][rand_y].size() <= 0)
					active_position[rand_x][rand_y] = 0;
			}
			else active_position[rand_x][rand_y] = 0;

			if(al_get_time() - time_started_thread > MAX_THREAD_TIME)
			{
				failed = 1;
				break;
			}
		}

		int test_sudoku[SIZE][SIZE];
		for(int x = 0; x < SIZE; ++x)
			for(int y = 0; y < SIZE; ++y)
				test_sudoku[x][y] = final_sudoku[x][y];

		if(failed)end = 1;
		else if(CanSolve(test_sudoku) && SolveSudoku(test_sudoku) && IsSolved(test_sudoku))
		{
			for(int x = 0; x < 9; ++x)
				for(int y = 0; y < 9; ++y)
					sudoku[x][y] = final_sudoku[x][y];
			end = 1;
		}
	}
	while((!end) && (!failed));

	done = 1; // now main function is ready to show result output
	loading = 0;
	return (!failed);
}
// ___ > FUNCTIONS FOR SUDOKU

// ___ < SWAPPING GENERATOR FUNCTIONS
void SwapRows(int sudoku[SIZE][SIZE], int row1, int row2)
{
	for(int col = 0; col < 9; col++)
	{
		int temp = sudoku[row1][col];
		sudoku[row1][col] = sudoku[row2][col];
		sudoku[row2][col] = temp;
	}
}
void SwapRowsGroup(int sudoku[SIZE][SIZE], int row_group1, int row_group2)
{
	for(int i = 0; i < 3; ++i)
		SwapRows(sudoku, (row_group1 * 3) + i, (row_group2 * 3) + i);
}
void SwapColums(int sudoku[SIZE][SIZE], int col1, int col2)
{
	for(int row = 0; row < 9; row++)
	{
		int temp = sudoku[row][col1];
		sudoku[row][col1] = sudoku[row][col2];
		sudoku[row][col2] = temp;
	}
}
void SwapColumsGroup(int sudoku[SIZE][SIZE], int col_group1, int col_group2)
{
	for(int i = 0; i < 3; ++i)
		SwapColums(sudoku, (col_group1 * 3) + i, (col_group2 * 3) + i);
}
void SwapRegions(int sudoku[SIZE][SIZE], int begin_row1, int begin_row2, int begin_col1, int begin_col2)
{
	for(int row = 0; row < 3; ++row)
	{
		for(int col = 0; col < 3; ++col)
		{
			int temp = sudoku[row + begin_row1][col + begin_col1];
			sudoku[row + begin_row1][col + begin_col1] = sudoku[row + begin_row2][col + begin_col2];
			sudoku[row + begin_row2][col + begin_col2] = temp;
		}
	}
}
void SwappingGenerator(int sudoku[SIZE][SIZE], int clues)
{
	// todo: give an option to use it
	do
	{
		FillSudoku(sudoku, 0);
		SolveSudoku(sudoku);

		int num_swap = rand() % 20 + 20;
		while(num_swap)
		{
			int n = rand() % 5;
			switch(n)
			{
				case 0: SwapRows(sudoku, rand() % SIZE, rand() % SIZE); break;
				case 1: SwapRowsGroup(sudoku, rand() % 3, rand() % 3); break;
				case 2: SwapColums(sudoku, rand() % SIZE, rand() % SIZE); break;
				case 3: SwapColumsGroup(sudoku, rand() % 3, rand() % 3); break;
				case 4: SwapRegions(sudoku, (rand() % 3) * 3, (rand() % 3) * 3, (rand() % 3) * 3, (rand() % 3) * 3); break;
				default:break;
			}
			--num_swap;
		}

		int num_clues = SIZE * SIZE;
		while(num_clues > clues)
		{
			int x = rand() % SIZE;
			int y = rand() % SIZE;
			if(sudoku[x][y] != 0)
			{
				--num_clues;
				sudoku[x][y] = 0;
			}
		}
	}
	while(!CanSolve(sudoku));
}
// ___ > SWAPPING GENERATOR FUNCTIONS

int main()
{
	srand(time(0));

	if(!al_init())return -1;
	if(!al_init_native_dialog_addon())return -1;
	if(!al_init_primitives_addon())return -1;
	if(!al_init_font_addon())return -1;
	if(!al_init_image_addon())return -1;
	if(!al_install_keyboard())return -1;
	if(!al_install_mouse())return -1;

	al_set_app_name("Sudoku App");
	al_set_exe_name("Sudoku App");
	al_set_org_name("PAULINEK");
	al_set_new_display_flags(ALLEGRO_RESIZABLE);

	int error = 0;

	ALLEGRO_DISPLAY* display = al_create_display(DISPLAY_W, DISPLAY_H);
	if(!display)error = 1;
	ALLEGRO_EVENT_QUEUE* event_queue = al_create_event_queue();
	if(!event_queue)error = 2;
	ALLEGRO_TIMER* timer = al_create_timer(1 / 20.f);
	if(!timer)error = 3;
	ALLEGRO_FONT* font = al_create_builtin_font();
	if(!font)error = 4;
	ALLEGRO_BITMAP* bitmap_sudoku_logo = al_load_bitmap("bitmap_sudoku_logo.png");
	if(!bitmap_sudoku_logo)error = 5;

	if(!error)
	{
		ALLEGRO_MONITOR_INFO info;
		al_get_monitor_info(0, &info);

		al_set_display_icon(display, bitmap_sudoku_logo);
		al_set_window_title(display, "Sudoku App");
		al_set_window_position(display, (info.x2 / 2.f) - ((info.x2 * 0.5f) / 2.f), (info.y2 / 2.f) - ((info.y2 * 0.75f) / 2.f));

		al_resize_display(display, (info.x2 * 0.5f), (info.y2 * 0.75f));
		TransformDisplay(display);

		al_register_event_source(event_queue, al_get_display_event_source(display));
		al_register_event_source(event_queue, al_get_timer_event_source(timer));
		al_register_event_source(event_queue, al_get_mouse_event_source());
		al_register_event_source(event_queue, al_get_keyboard_event_source());

		int end_app = 0;
		int draw_app = 0;
		int sudoku[SIZE][SIZE] = {0};
		int last_actived_box_indexes[2] = {1,1};
		int active_area = 0;
		float load_circle[2] = {0.f,0.f};

		Grid grid = {25.f,100.f,625.f,700.f};
		Box boxes[SIZE][SIZE];
		Box* box_active = 0; // pointer to a box to which user can type number
		Button buttons[NUMBER_BUTTONS];
		InputBox input_box;

		std::string str_output[2] = {"output 1","output 2"};
		std::future<bool>future_generator;

		// init all boxes
		{
			// note: without adding this gap to cx and cy text is not centered... but why???
			float text_gap = 60.f / 19.f;
			float box_px1 = 32.f;
			float box_py1 = 106.f;
			for(int y = 0; y < SIZE; ++y)
			{
				for(int x = 0; x < SIZE; ++x)
				{
					boxes[x][y].x = x;
					boxes[x][y].y = y;
					boxes[x][y].region = x / 3 * 3 + y / 3;
					boxes[x][y].px1 = box_px1;
					boxes[x][y].py1 = box_py1;
					boxes[x][y].px2 = box_px1 + 60.f;
					boxes[x][y].py2 = box_py1 + 60.f;
					boxes[x][y].cx = box_px1 + 30.f + text_gap;
					boxes[x][y].cy = box_py1 + 30.f + text_gap;

					box_py1 += 66.f;
				}
				box_px1 += 66.f;
				box_py1 = 106.f;
			}
		}
		// init all buttons
		{
			float button_py = 100.f;
			float button_h = (600.f / float(NUMBER_BUTTONS));
			for(int i = 0; i < NUMBER_BUTTONS; ++i)
			{
				buttons[i].px1 = 650.f;
				buttons[i].py1 = button_py;
				buttons[i].px2 = 975.f;
				buttons[i].py2 = button_py + button_h;
				buttons[i].cx = 812.5f;
				buttons[i].cy = button_py + (button_h / 2.f);
				buttons[i].text_w = 250.f;

				button_py += button_h;
			}
			buttons[BUTTON::SOLVE].text = "solve";
			buttons[BUTTON::RESET].text = "reset";
			buttons[BUTTON::GENERATE].text = "generate";
			buttons[BUTTON::SAVE].text = "save";
			buttons[BUTTON::LOAD].text = "load";
		}
		input_box.text = "sudoku_file_name";

		// ___ < MAIN LOOP
		al_start_timer(timer);
		while(!end_app)
		{
			ALLEGRO_EVENT event;
			al_wait_for_event(event_queue, &event);

			// ___ < SWITCH EVENT
			switch(event.type)
			{
				case ALLEGRO_EVENT_DISPLAY_CLOSE:end_app = 1; break;
				case ALLEGRO_EVENT_DISPLAY_RESIZE:TransformDisplay(display); break;
				case ALLEGRO_EVENT_TIMER:
				{
					draw_app = 1;

					if(loading)
					{
						load_circle[0] += 2.f;
						load_circle[1] += 0.0005f;
						if(load_circle[0] > 50.f)
						{
							load_circle[0] = 0.f;
							load_circle[1] = 0.f;
						}
					}
					if(done)
					{
						done = 0;
						str_output[0] = str_output[1];
						UpdateBoxes(boxes, sudoku);
						if(failed)
						{
							FillBoxes(boxes, -1);
							FillSudoku(sudoku, -1);
							str_output[1] = "failed to generate! time exceed!";
						}
						else str_output[1] = "generated successfully!";
					}
				}break;
				case ALLEGRO_EVENT_MOUSE_AXES:
				{
					float mouse_x = event.mouse.x;
					float mouse_y = event.mouse.y;
					TransformMouse(&mouse_x, &mouse_y);

					// check if mouse is hovering any box
					for(int y = 0; y < SIZE; ++y)
						for(int x = 0; x < SIZE; ++x)
							if(mouse_x > boxes[x][y].px1 && mouse_x < boxes[x][y].px2
							   && mouse_y > boxes[x][y].py1 && mouse_y < boxes[x][y].py2)
								boxes[x][y].hovered = 1;
							else boxes[x][y].hovered = 0;

					// check if mouse is hovering any button
					for(int i = 0; i < NUMBER_BUTTONS; ++i)
					{
						if(mouse_x > buttons[i].px1 && mouse_x < buttons[i].px2
						   && mouse_y > buttons[i].py1 && mouse_y < buttons[i].py2)
						{
							buttons[i].hovered = 1;
							buttons[i].text_w = 300.f;
						}
						else
						{
							buttons[i].hovered = 0;
							buttons[i].text_w = 250.f;
						}
					}

					// check if mouse is hovering the input box
					if(mouse_x > 25.f && mouse_x < 975.f && mouse_y > 725.f && mouse_y < 775.f)input_box.hovered = 1;
					else input_box.hovered = 0;
				}break;
				case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
				{
					active_area = 0;
					box_active = 0;
					input_box.active = 0;

					// do not allow to do something while generating sudoku
					if(!loading)
					{
						for(int y = 0; y < SIZE && !active_area; ++y)
						{
							for(int x = 0; x < SIZE; ++x)
							{
								if(boxes[x][y].hovered)
								{
									// activate box that is currently touched
									box_active = &(boxes[x][y]);
									last_actived_box_indexes[0] = box_active->x;
									last_actived_box_indexes[1] = box_active->y;
									active_area = ACTIVE_AREA::GRID;
									break;
								}
							}
						}
						if(buttons[BUTTON::SOLVE].hovered)
						{
							str_output[0] = str_output[1];
							str_output[1] = "solving started...";

							// actually it is not another async function 
							// since the function to solve sudoku is fast enough (AFAIK)
							// but without it the function will always return 0 (false)
							time_started_thread = al_get_time();
							UpdateSudoku(sudoku, boxes); // move values from boxes to sudoku
							if(CanSolve(sudoku))
							{
								// try to solve
								// it does not ensure that sudoku with given numbers will be solved
								SolveSudoku(sudoku);
								if(!IsSolved(sudoku))
								{
									str_output[0] = str_output[1];
									str_output[1] = "failed to solve!";
									FillBoxes(boxes, -1);
								}
								else
								{
									str_output[0] = str_output[1];
									str_output[1] = "solved successfully!";
									UpdateBoxes(boxes, sudoku);
								}
							}
							else
							{
								str_output[0] = str_output[1];
								str_output[1] = "invalid sudoku!";
								FillBoxes(boxes, -1);
							}
						}
						else if(buttons[BUTTON::RESET].hovered)
						{
							for(int y = 0; y < SIZE; ++y)
								for(int x = 0; x < SIZE; ++x)
									boxes[x][y].number = 0;
							str_output[0] = str_output[1];
							str_output[1] = "sudoku reset!";
						}
						else if(buttons[BUTTON::GENERATE].hovered)
						{
							str_output[0] = str_output[1];
							str_output[1] = "generating started...";
							future_generator = (std::async(GenerateSudoku, sudoku, rand() % NUM_CLUES + MIN_CLUES));
							load_circle[0] = load_circle[1] = 0.f;
						}
						else if(buttons[BUTTON::SAVE].hovered)
						{
							UpdateSudoku(sudoku, boxes);
							str_output[0] = str_output[1];
							if(!SaveSudoku(sudoku, input_box.text))
								str_output[1] = "Failed to save to " + input_box.text + ".txt";
							else
								str_output[1] = "Successfully saved to " + input_box.text + ".txt";
						}
						else if(buttons[BUTTON::LOAD].hovered)
						{
							str_output[0] = str_output[1];
							if(!LoadSudoku(sudoku))
								str_output[1] = "Failed to load from the file";
							else
							{
								str_output[1] = "Successfully loaded from the file";
								UpdateBoxes(boxes, sudoku);
							}
						}
						else if(input_box.hovered)
						{
							input_box.active = 1;
							active_area = ACTIVE_AREA::INPUT;
						}
					}
				}break;
				case ALLEGRO_EVENT_KEY_DOWN:
				{
					// ___ < SWITCH ACTIVE AREA
					switch(active_area)
					{
						case 0:break;
						case ACTIVE_AREA::GRID:
						{
							// ___ < SWITCH KEYCODES FOR GRID
							switch(event.keyboard.keycode)
							{
								case ALLEGRO_KEY_1:box_active->number = 1; break;
								case ALLEGRO_KEY_2:box_active->number = 2; break;
								case ALLEGRO_KEY_3:box_active->number = 3; break;
								case ALLEGRO_KEY_4:box_active->number = 4; break;
								case ALLEGRO_KEY_5:box_active->number = 5; break;
								case ALLEGRO_KEY_6:box_active->number = 6; break;
								case ALLEGRO_KEY_7:box_active->number = 7; break;
								case ALLEGRO_KEY_8:box_active->number = 8; break;
								case ALLEGRO_KEY_9:box_active->number = 9; break;
								case ALLEGRO_KEY_0:box_active->number = 0; break;
								case ALLEGRO_KEY_LEFT:
								{
									last_actived_box_indexes[1]--;
									if(last_actived_box_indexes[1] < 0)
									{
										last_actived_box_indexes[1] = SIZE - 1;
										last_actived_box_indexes[0]--;
										if(last_actived_box_indexes[0] < 0)
										{
											last_actived_box_indexes[0] = 0;
											last_actived_box_indexes[1] = 0;
										}
									}
									box_active = &(boxes[last_actived_box_indexes[0]][last_actived_box_indexes[1]]);
								}break;
								case ALLEGRO_KEY_RIGHT:
								{
									last_actived_box_indexes[1]++;
									if(last_actived_box_indexes[1] > SIZE - 1)
									{
										last_actived_box_indexes[1] = 0;
										last_actived_box_indexes[0]++;
										if(last_actived_box_indexes[0] > SIZE - 1)
										{
											last_actived_box_indexes[0] = SIZE - 1;
											last_actived_box_indexes[1] = SIZE - 1;
										}
									}
									box_active = &(boxes[last_actived_box_indexes[0]][last_actived_box_indexes[1]]);
								}break;
								case ALLEGRO_KEY_UP:
								{
									if(--last_actived_box_indexes[0] < 0)last_actived_box_indexes[0] = 0;
									box_active = &(boxes[last_actived_box_indexes[0]][last_actived_box_indexes[1]]);
								}break;
								case ALLEGRO_KEY_DOWN:
								{
									if(++last_actived_box_indexes[0] > SIZE - 1)last_actived_box_indexes[0] = SIZE - 1;
									box_active = &(boxes[last_actived_box_indexes[0]][last_actived_box_indexes[1]]);
								}break;

								default:break;
							}
							// ___ > SWITCH KEYCODES FOR GRID
						}break;
						case ACTIVE_AREA::INPUT:
						{
							char ch = 0;

							// ___ < SWITCH KEYCODES FOR INPUT BOX
							switch(event.keyboard.keycode)
							{
								case ALLEGRO_KEY_1:ch = '1'; break;
								case ALLEGRO_KEY_2:ch = '2'; break;
								case ALLEGRO_KEY_3:ch = '3'; break;
								case ALLEGRO_KEY_4:ch = '4'; break;
								case ALLEGRO_KEY_5:ch = '5'; break;
								case ALLEGRO_KEY_6:ch = '6'; break;
								case ALLEGRO_KEY_7:ch = '7'; break;
								case ALLEGRO_KEY_8:ch = '8'; break;
								case ALLEGRO_KEY_9:ch = '9'; break;
								case ALLEGRO_KEY_0:ch = '0'; break;
								case ALLEGRO_KEY_Q:ch = 'q'; break;
								case ALLEGRO_KEY_W:ch = 'w'; break;
								case ALLEGRO_KEY_E:ch = 'e'; break;
								case ALLEGRO_KEY_R:ch = 'r'; break;
								case ALLEGRO_KEY_T:ch = 't'; break;
								case ALLEGRO_KEY_Y:ch = 'y'; break;
								case ALLEGRO_KEY_U:ch = 'u'; break;
								case ALLEGRO_KEY_I:ch = 'i'; break;
								case ALLEGRO_KEY_O:ch = 'o'; break;
								case ALLEGRO_KEY_P:ch = 'p'; break;
								case ALLEGRO_KEY_A:ch = 'a'; break;
								case ALLEGRO_KEY_S:ch = 's'; break;
								case ALLEGRO_KEY_D:ch = 'd'; break;
								case ALLEGRO_KEY_F:ch = 'f'; break;
								case ALLEGRO_KEY_G:ch = 'g'; break;
								case ALLEGRO_KEY_H:ch = 'h'; break;
								case ALLEGRO_KEY_J:ch = 'j'; break;
								case ALLEGRO_KEY_K:ch = 'k'; break;
								case ALLEGRO_KEY_L:ch = 'l'; break;
								case ALLEGRO_KEY_Z:ch = 'z'; break;
								case ALLEGRO_KEY_X:ch = 'x'; break;
								case ALLEGRO_KEY_C:ch = 'c'; break;
								case ALLEGRO_KEY_V:ch = 'v'; break;
								case ALLEGRO_KEY_B:ch = 'b'; break;
								case ALLEGRO_KEY_N:ch = 'n'; break;
								case ALLEGRO_KEY_M:ch = 'm'; break;
								case ALLEGRO_KEY_MINUS:ch = '_'; break;
								case ALLEGRO_KEY_BACKSPACE:
								{
									if(input_box.text.size() > 0)input_box.text.pop_back();
								}break;

								default:break;
							}
							// ___ > SWITCH KEYCODES FOR INPUT BOX

							if((ch != 0) && (input_box.text.size() < MAX_FILE_NAME))
								input_box.text += ch;
						}break;

						default:break;
					}
					// ___ > SWITCH ACTIVE AREA
				}break;

				default:break;
			}
			// ___ > SWITCH EVENT

			if(draw_app && al_is_event_queue_empty(event_queue))
			{
				draw_app = 0;

				al_clear_to_color(COLOR_BACKGROUND);

				DrawText(font, 950.f, 50.f, 500.f, 50.f, 1, COLOR_DARK, "Sudoku App");

				// draw sudoku 
				{
					al_draw_filled_rectangle(grid.px1, grid.py1, grid.px2, grid.py2, COLOR_DARK);

					// draw a rectangle for each region to easily differentiate them 
					{
						float rect_px = 31.f;
						float rect_py = 106.f;
						for(int i = 0; i < 3; ++i)
						{
							for(int j = 0; j < 3; ++j)
							{
								if(!((j + i) % 2))
									al_draw_filled_rectangle(rect_px, rect_py, rect_px + 192.5f,
															 rect_py + 192.5f, COLOR_REGION1);
								else
									al_draw_filled_rectangle(rect_px, rect_py, rect_px + 192.5f,
															 rect_py + 192.5f, COLOR_REGION2);
								rect_px += 198.5f;
							}
							rect_py += 198.5f;
							rect_px = 31.f;
						}
					}

					// draw boxes with numbers
					{
						for(int y = 0; y < SIZE; ++y)
						{
							for(int x = 0; x < SIZE; ++x)
							{
								al_draw_filled_rectangle(boxes[x][y].px1, boxes[x][y].py1, boxes[x][y].px2, boxes[x][y].py2,
														 boxes[x][y].hovered ? COLOR_HOVERED : COLOR_BACKGROUND);
								DrawText(font, boxes[x][y].hovered ? 60.f : 50.f, boxes[x][y].hovered ? 60.f : 50.f,
										 boxes[x][y].cx, boxes[x][y].cy, 1, COLOR_DARK, std::to_string(boxes[x][y].number).c_str());
								if(box_active)
								{
									if(boxes[x][y].number == box_active->number)
									{
										if((boxes[x][y].x == box_active->x) || (boxes[x][y].y == box_active->y)
										   || (boxes[x][y].region == box_active->region))
											al_draw_rectangle(boxes[x][y].px1, boxes[x][y].py1, boxes[x][y].px2, boxes[x][y].py2,
															  COLOR_DUPLICATE_NUMBER, 10.f);
										else
											al_draw_rectangle(boxes[x][y].px1, boxes[x][y].py1, boxes[x][y].px2, boxes[x][y].py2,
															  COLOR_SAME_NUMBER, 10.f);
									}
								}
							}
						}
					}
				}

				// highlight active box
				if(box_active)al_draw_filled_rectangle(box_active->px1, box_active->py1, box_active->px2, box_active->py2, COLOR_ACTIVED);

				// draw buttons
				{
					for(int i = 0; i < NUMBER_BUTTONS; ++i)
					{
						al_draw_rectangle(buttons[i].px1, buttons[i].py1,
										  buttons[i].px2, buttons[i].py2,
										  COLOR_DARK, 6.f);
						if(buttons[i].hovered)
							al_draw_filled_rectangle(buttons[i].px1, buttons[i].py1,
													 buttons[i].px2, buttons[i].py2,
													 COLOR_HOVERED);
						DrawText(font, buttons[i].text_w, 1000.f, buttons[i].cx, buttons[i].cy, 1,
								 COLOR_DARK, buttons[i].text);
					}
				}

				// draw input box
				{
					al_draw_rectangle(25.f, 725.f, 975.f, 775.f, COLOR_DARK, 6.f);
					if(input_box.hovered)
						al_draw_filled_rectangle(25.f, 725.f, 975.f, 775.f, COLOR_HOVERED);
					if(input_box.active)
					{
						al_draw_filled_rectangle(25.f, 725.f, 975.f, 775.f, COLOR_ACTIVED);
						float px = 110.f + (input_box.text.size() * 30.f);
						al_draw_filled_rectangle(px, 725.f, px + 10.f, 775.f,
												 al_map_rgba(0, 0, 0, al_get_timer_count(timer) * 20));
					}
					DrawText(font, 700.f, 30.f, 100.f, 735.f, ALLEGRO_ALIGN_LEFT, COLOR_DARK, input_box.text.c_str());
				}

				// draw output box
				{
					al_draw_rectangle(25.f, 800.f, 975.f, 975.f, COLOR_DARK, 6.f);
					DrawText(font, 500.f, 30.f, 500.f, 825.f, ALLEGRO_ALIGN_CENTER, COLOR_DARK, str_output[0].c_str());
					DrawText(font, 900.f, 70.f, 500.f, 900.f, ALLEGRO_ALIGN_CENTER, COLOR_DARK, str_output[1].c_str());
				}

				if(loading)
				{
					al_draw_filled_rectangle(grid.px1, grid.py1, grid.px2, grid.py2, al_map_rgba(0, 0, 0, 132));
					al_draw_filled_rectangle(650.f, 100.f, 975.f, 700.f, al_map_rgba(0, 0, 0, 132));
					al_draw_filled_circle(grid.px1 + 300.f, grid.py1 + 300.f, load_circle[0], al_map_rgba_f(1.f, 1.f, 0, load_circle[1]));
				}

				al_flip_display();
			}
		}
		al_stop_timer(timer);
		// ___ > MAIN LOOP

		if(future_generator.valid())
			future_generator.get();
	}

	al_destroy_display(display);
	al_destroy_event_queue(event_queue);
	al_destroy_timer(timer);
	al_destroy_font(font);
	al_destroy_bitmap(bitmap_sudoku_logo);

	return error;
}
