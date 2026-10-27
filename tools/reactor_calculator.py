#!/usr/bin/env python3
"""Reactor Incremental — area and heat balance calculator (Tkinter, stdlib only).

All fuel rods occupy ONE cell each. Any additional cells needed for
reflectors/supports/etc. are entered manually in 'Extra occupied cells'.
"""
import tkinter as tk
from tkinter import ttk

DEFAULTS = {
    'width': '19', 'height': '20', 'reserved': '30',
    'groups': '36', 'capacitors': '160',
    'small': '1', 'medium': '2', 'large': '1', 'extra': '3',
    'group_base': '72.8', 'cap_bonus': '22.4',
    'small_heat': '1250', 'medium_heat': '20000', 'large_heat': '90000',
}

LABELS = [
    ('width', 'Field width'), ('height', 'Field height'),
    ('reserved', 'Reserved / unavailable cells'),
    ('groups', 'Outlet + 4 Vent groups (5 cells/group)'),
    ('capacitors', 'Capacitors (1 cell each)'),
    ('small', 'Small fuel rods (1 cell each)'),
    ('medium', 'Medium fuel rods (1 cell each)'),
    ('large', 'Large fuel rods (1 cell each)'),
    ('extra', 'EXTRA occupied cells (reflectors, etc.)'),
    ('group_base', 'Base cooling / group'),
    ('cap_bonus', 'Bonus / capacitor / group'),
    ('small_heat', 'Heat / small rod'),
    ('medium_heat', 'Heat / medium rod'),
    ('large_heat', 'Heat / large rod'),
]

root = tk.Tk()
root.title('Reactor — Cells & Heat Calculator')
root.geometry('645x750')
root.minsize(550, 660)
frame = ttk.Frame(root, padding=14)
frame.pack(fill='both', expand=True)

vars_ = {name: tk.StringVar(value=DEFAULTS[name]) for name in DEFAULTS}
for i, (key, label) in enumerate(LABELS):
    ttk.Label(frame, text=label).grid(row=i, column=0, sticky='w', pady=4, padx=(0, 12))
    ttk.Entry(frame, textvariable=vars_[key], width=19).grid(row=i, column=1, sticky='ew', pady=4)

frame.columnconfigure(0, weight=1)
frame.columnconfigure(1, weight=0)
separator_row = len(LABELS)
ttk.Separator(frame).grid(row=separator_row, column=0, columnspan=2, sticky='ew', pady=12)
result = tk.StringVar()
result_label = ttk.Label(frame, textvariable=result, justify='left', font=('Consolas', 11))
result_label.grid(row=separator_row+1, column=0, columnspan=2, sticky='nw')
status = tk.StringVar()
ttk.Label(frame, textvariable=status, wraplength=590, justify='left').grid(
    row=separator_row+2, column=0, columnspan=2, sticky='w', pady=(12, 0)
)

def recalc(*_):
    try:
        vals = {k: float(v.get().strip().replace(',', '.')) for k, v in vars_.items()}
        integers = ('width', 'height', 'reserved', 'groups', 'capacitors', 'small', 'medium', 'large', 'extra')
        if any(vals[k] < 0 or not vals[k].is_integer() for k in integers):
            raise ValueError('Cell and component counts must be nonnegative integers.')
        if any(vals[k] < 0 for k in vals):
            raise ValueError('Values cannot be negative.')
        total = int(vals['width'] * vals['height'])
        usable = total - int(vals['reserved'])
        fuel_cells = int(vals['small'] + vals['medium'] + vals['large'])
        used = int(5 * vals['groups'] + vals['capacitors'] + fuel_cells + vals['extra'])
        free = usable - used
        cooling = vals['groups'] * (vals['group_base'] + vals['cap_bonus'] * vals['capacitors'])
        heat = (vals['small'] * vals['small_heat'] + vals['medium'] * vals['medium_heat'] + vals['large'] * vals['large_heat'])
        margin = cooling - heat
        def f(x): return f'{x:,.2f}'.rstrip('0').rstrip('.')
        result.set(
            f'Field size:        {total:,} cells\n'
            f'Usable:            {usable:,} cells\n'
            f'Cooling groups:    {int(vals["groups"]):,} × 5 = {int(5 * vals["groups"]):,} cells\n'
            f'Capacitors:        {int(vals["capacitors"]):,} cells\n'
            f'Fuel (all types):  {fuel_cells:,} cells\n'
            f'Additional cells:  {int(vals["extra"]):,} cells\n'
            f'Used / usable:     {used:,} / {usable:,}\n'
            f'Free:              {free:,} cells\n\n'
            f'Generated heat:    {f(heat)} / tick\n'
            f'Cooling capacity:  {f(cooling)} / tick\n'
            f'Cooling reserve:   {f(margin)} / tick'
        )
        problems = []
        if usable < 0 or free < 0: problems.append('OVER CAPACITY: Not enough free cells.')
        if margin < 0: problems.append('OVERHEATING: Heat exceeds theoretical cooling.')
        if not problems: problems.append('Theoretical balance passes area and heat checks.')
        problems.append('Note: geometry, Outlet throughput, adjacency and real game tick behavior are not simulated.')
        status.set('  '.join(problems))
    except (ValueError, OverflowError) as exc:
        result.set('Enter valid numbers in all fields.')
        status.set(str(exc))

for v in vars_.values(): v.trace_add('write', recalc)
recalc()
root.mainloop()
