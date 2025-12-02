#pragma once


struct TickableResetFlag
{

    bool FlaggedForReset = false;
    bool NeedsResetting = false;

    void FlagForReset() { FlaggedForReset = true; }
    void Tick()
    {
        if (FlaggedForReset)
        {
            FlaggedForReset = false;
            NeedsResetting = true;
        }
        else
        {
            NeedsResetting = false;
        }
    };

    bool NeedsReset() const { return NeedsResetting; }
};