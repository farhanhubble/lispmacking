"""Architecture used by the opening article; prose stays in the Markdown."""
from diagramkit import Diagram, BLUE, AMBER, VIOLET, TEAL, MUTED


def architecture():
    d = Diagram("Inside the Lisp machine", "QEMU emulates an RV64 CPU, RAM and UART. Inside the guest, Lisp compiler and assembler programs are interpreted by the C evaluator above the bare-metal platform adapter. The tools emit instructions into a RAM code page; bootstrap:call-native enters that code on the emulated CPU. All guest code currently runs in machine mode, without a guest OS.", 780, "01", "", compact=True)
    # QEMU is the enclosing machine, not a guest software layer.
    d.rect(36, 75, 728, 637, "#f2f5f8", "#64748b", radius=18)
    d.text(58, 110, "QEMU", 25, "#334155", bold=True)
    d.text(151, 109, "virt · emulated RISC-V machine", 18, MUTED)
    d.text(58, 153, "OUR BARE-METAL SOFTWARE", 13, MUTED, bold=True)
    d.text(58, 176, "All execution in machine mode", 15, MUTED)

    # Spatial stacking encodes dependency. Touching layers are not time steps.
    d.layer(66, 210, 420, 76, VIOLET, "Lisp compiler + assembler", "lisp/compiler.lisp  ·  lisp/assembler.lisp")
    d.text(82, 313, "interpreted by", 14, BLUE.ink)
    d.arrow(450, 292, 450, 324, BLUE.ink)
    d.layer(66, 332, 420, 76, BLUE, "Portable C evaluator", "objects  ·  reader  ·  evaluator  ·  printer")
    d.arrow(276, 416, 276, 440, TEAL.ink)
    d.layer(66, 448, 420, 76, TEAL, "Bare-metal platform adapter", "arena allocator  ·  UART I/O  ·  machine operations")
    d.text(82, 548, "boot/start.S / kernel_main", 15, MUTED)

    # The emitted artifact is beside the interpreter, not another interpreter layer.
    d.rect(558, 292, 180, 154, VIOLET.wash, VIOLET.ink, radius=7)
    d.text(575, 320, "Code page", 21, VIOLET.ink, bold=True)
    d.text(575, 343, "in guest RAM", 15, VIOLET.ink)
    for row in range(3):
        for column in range(6):
            d.rect(576+column*24, 363+row*20, 17, 12,
                   VIOLET.ink if (row+column)%3 else "#b59bd9", radius=2)
    d.route([(486,248),(648,248),(648,284)], VIOLET.ink)
    d.text(530, 233, "emit RV64", 16, VIOLET.ink)

    # Hardware symbols sit within the same emulated-machine boundary.
    d.rect(58, 584, 684, 108, "#fff4d9", AMBER.ink, radius=10)
    d.text(76, 608, "EMULATED HARDWARE", 12, AMBER.ink, bold=True)
    for x in (268, 504):
        d.line(x, 621, x, 676, "#dbc799", 1)
    d.rect(79, 632, 42, 30, AMBER.wash, AMBER.ink, radius=3)
    d.text(85, 653, ">_", 17, AMBER.ink, bold=True)
    d.text(139, 651, "UART", 22, AMBER.ink, bold=True)
    d.rect(294, 632, 48, 28, AMBER.wash, AMBER.ink, radius=3)
    for x in (300,313,326):
        d.rect(x, 638, 9, 15, AMBER.ink, radius=1)
    d.text(359, 651, "RAM", 22, AMBER.ink, bold=True)
    d.chip(530, 628, 40, AMBER)
    d.text(588, 653, "RV64 CPU", 21, AMBER.ink, bold=True)
    d.arrow(276, 555, 276, 576, TEAL.ink)
    d.arrow(718, 454, 718, 576, AMBER.ink)
    d.text(544, 481, "native call", 16, AMBER.ink)
    d.text(544, 505, "executes on CPU", 14, AMBER.ink)
    d.text(36, 744, "QEMU runs on the host. No guest OS sits beneath our code.", 18, MUTED)
    return d


def diagrams():
    return {"bare-metal-enlightenment": architecture()}
