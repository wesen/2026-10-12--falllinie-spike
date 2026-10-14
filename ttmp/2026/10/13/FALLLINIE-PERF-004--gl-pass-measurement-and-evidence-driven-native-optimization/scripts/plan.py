#!/usr/bin/env python3
"""Print a small balanced order; does NOT execute it or contact any device."""
static = ['static-off', 'static-passes', 'static-single', 'static-bypass', 'static-res']
order = static + list(reversed(static)) + ['live-off', 'live-passes', 'live-passes', 'live-off']
print('order,case,repetition,phase')
seen = {}
for i, case in enumerate(order, 1):
    seen[case] = seen.get(case, 0)+1
    print(f'{i:02},{case},{seen[case]},{"static" if case.startswith("static") else "live"}')
