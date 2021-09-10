#pragma once

// --==== Types ====--

typedef struct rg_engine rg_engine;

// --==== Engine ====--

rg_engine *rg_create_engine(void);
void rg_destroy_engine(rg_engine **engine);
void rg_engine_run_main_loop(rg_engine *engine);