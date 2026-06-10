#pragma once

enum BotState { CALM, CUDDLE, HIT, FALLEN };

void expressionsBegin();
void showState(BotState s);
void updateIdle();