#pragma once
#include <cstdio>
namespace d2rt { class Cpu; }
class StartupServices;
bool run_batch_checks(d2rt::Cpu&, StartupServices&, FILE*);
