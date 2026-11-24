/* Regression tests for fractional VST3 bars and host-provided bar starts. */
#include "../distrho/src/DistrhoVST3TimePosition.hpp"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace DISTRHO;
static int failures = 0;
static void check(bool ok, const char *text)
{
    if (!ok)
    {
        std::fprintf(stderr, "FAIL: %s\n", text);
        ++failures;
    }
}
static bool near(double a, double b) { return std::fabs(a - b) < 1.e-7; }
static v3_process_context context(int n, int d, double q, double start)
{
    v3_process_context c = {};
    c.state = V3_PROCESS_CTX_PROJECT_TIME_VALID | V3_PROCESS_CTX_TIME_SIG_VALID |
              V3_PROCESS_CTX_BAR_POSITION_VALID | V3_PROCESS_CTX_TEMPO_VALID |
              V3_PROCESS_CTX_PLAYING;
    c.time_sig_numerator = n;
    c.time_sig_denom = d;
    c.project_time_quarters = q;
    c.bar_position_quarters = start;
    c.bpm = 190.;
    return c;
}
static void timingTests()
{
    TimePosition::BarBeatTick b;
    fillVST3BarBeatTick(context(19, 16, 4.75, 4.75), b);
    check(b.valid && b.bar == 2 && b.beat == 1 && near(b.tick, 0.),
          "19/16 bar boundary is 4.75 quarters");
    fillVST3BarBeatTick(context(19, 16, 4.875, 4.75), b);
    check(b.beat == 1 && near(b.tick, 960.), "19/16 half of a sixteenth-note beat");
    fillVST3BarBeatTick(context(1, 16, .25, .25), b);
    check(b.valid && b.bar == 2 && b.beat == 1, "sub-quarter bar has no integer-division-by-zero");
    fillVST3BarBeatTick(context(17, 32, 2.125, 2.125), b);
    check(b.valid && b.bar == 2 && b.beat == 1, "17/32 fractional bar");
    fillVST3BarBeatTick(context(7, 8, 4.25, 4.), b);
    check(b.beat == 1 && near(b.tick, 960.) && near(b.barStartTick, 15360.),
          "use host bar start following a meter change");
    fillVST3BarBeatTick(context(19, 16, -.125, -4.75), b);
    check(b.valid && b.bar == 0 && b.beat == 19 && near(b.tick, 960.), "negative PPQ count-in");
    auto c = context(19, 16, 4.75, 0.);
    c.state &= ~V3_PROCESS_CTX_BAR_POSITION_VALID;
    fillVST3BarBeatTick(c, b);
    check(b.bar == 2 && b.beat == 1, "fallback without host bar-start field");
    c = context(19, 0, 0., 0.);
    fillVST3BarBeatTick(c, b);
    check(!b.valid, "reject invalid denominator");
    c = context(19, 16, std::numeric_limits<double>::quiet_NaN(), 0.);
    fillVST3BarBeatTick(c, b);
    check(!b.valid, "reject non-finite PPQ");
    c = context(4, 4, 0., 0.);
    c.state = 0;
    fillVST3BarBeatTick(c, b);
    check(!b.valid, "reject missing timing flags");
}
int main()
{
    timingTests();
    std::printf("%s: VST3 bar/beat/tick conversion\n", failures ? "FAIL" : "PASS");
    return failures ? 1 : 0;
}
