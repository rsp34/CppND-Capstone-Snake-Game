#include "game.h"
#include <iostream>
#include <thread>
#include <mutex>
#include "SDL.h"

Game::Game(std::size_t grid_width, std::size_t grid_height)
    : snake(grid_width, grid_height),
      engine(dev()),
      random_w(0, static_cast<int>(grid_width - 1)),
      random_h(0, static_cast<int>(grid_height - 1)) {
  PlaceFood();
}

Game::~Game() {
    bad_food.Cancel();
    if (bad_food_timer.joinable()) {
        bad_food_timer.join();
    }
}

void Game::Run(Controller const &controller, Renderer &renderer,
               std::size_t target_frame_duration) {
  Uint32 title_timestamp = SDL_GetTicks();
  Uint32 frame_start;
  Uint32 frame_end;
  Uint32 frame_duration;
  int frame_count = 0;
  bool is_bad_food_active = false;
  bool running = true;

  while (running) {
    frame_start = SDL_GetTicks();

    // Input, Update, Render - the main game loop.
    controller.HandleInput(running, snake);
    Update();
    renderer.Render(snake, food, bad_food);

    frame_end = SDL_GetTicks();

    // Keep track of how long each loop through the input/update/render cycle
    // takes.
    frame_count++;
    frame_duration = frame_end - frame_start;

    // After every second, update the window title.
    if (frame_end - title_timestamp >= 1000) {
      renderer.UpdateWindowTitle(score, frame_count);
      frame_count = 0;
      title_timestamp = frame_end;
    }

    // If the time for this frame is too small (i.e. frame_duration is
    // smaller than the target ms_per_frame), delay the loop to
    // achieve the correct frame rate.
    if (frame_duration < target_frame_duration) {
      SDL_Delay(target_frame_duration - frame_duration);
    }
  }
}

void Game::PlaceFood() {
  int x, y;
  while (true) {
    x = random_w(engine);
    y = random_h(engine);
    // Check that the location is not occupied by a snake item before placing
    // food.
    if (!snake.SnakeCell(x, y)) {
      food.x = x;
      food.y = y;
      return;
    }
  }
}

void Game::PlaceBadFood() {
    int x, y;
    while (true) {
        x = random_w(engine);
        y = random_h(engine);
        // Check that the location is not occupied by a snake item before placing
        // food.
        if (!snake.SnakeCell(x, y) && (x != food.x && y != food.y)) {
            if (bad_food_timer.joinable())
            {
                bad_food.Cancel();
                bad_food_timer.join();
			}
            bad_food.Place(x, y);
            bad_food_timer = std::thread(&BadFood::BadFoodTimer, &bad_food);
            return;
        }
    }
}

void Game::Update() {
    if (!snake.alive) return;

    snake.Update();

    int new_x = static_cast<int>(snake.head_x);
    int new_y = static_cast<int>(snake.head_y);

    // Check if there's bad food over here
    if (bad_food.IsEaten(new_x, new_y))
    {
        score--;

		bad_food.Remove();
        bad_food.Cancel();
        if (bad_food_timer.joinable()) bad_food_timer.join();

        snake.GrowBody();
		snake.speed += speed_increment;
	}

    // Check if there's food over here
    if (food.x == new_x && food.y == new_y) {
        
        // Update the score
        score++;

        // Place any more food
        PlaceFood();        
        if (score > 0 && !bad_food.IsActive())
        {
            PlaceBadFood();
		}

        // Grow snake and increase speed.
        snake.GrowBody();
        snake.speed += speed_increment;   
    }
}

int Game::GetScore() const { return score; }
int Game::GetSize() const { return snake.size; }