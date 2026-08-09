#pragma once

class CSandMan;

namespace DimSumSurprise {

// Schedules one startup draw.  The helper reads only an application-data cache
// populated from the public dim-sum-photos catalog; it never downloads or
// vendors an image in the application repository.
void schedule(CSandMan* parent);

}
