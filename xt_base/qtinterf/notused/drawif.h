
// The QTcanvas drawing interface.
//
class qtinterf::QTdrawIf : public QWidget
{
public:
    static QTdrawIf *new_draw_interface(DrawType, bool, QWidget*);

    QTdrawIf(QWidget *prnt = 0) : QWidget(prnt) { }
    virtual ~QTdrawIf() { }

    virtual QPixmap *pixmap() = 0; 

    virtual void switch_to_pixmap2(QPixmap* = 0) = 0;
    virtual void switch_from_pixmap2(int, int, int, int, int, int) = 0;
    virtual void clear_pixmap2() = 0;
    virtual void set_draw_to_pixmap(QPixmap*) = 0;
    virtual void set_overlay_mode(bool) = 0;
    virtual void create_overlay_backg() = 0;
    virtual void erase_last_overlay() = 0;
    virtual void set_clipping(int, int, int, int) = 0;
    virtual void refresh(int, int, int, int) = 0;
    virtual void refresh() = 0;
    virtual void update(int, int, int, int) = 0;
    virtual void update() = 0;
    virtual void clear() = 0;
    virtual void clear_area(int, int, int, int) = 0;

    virtual void set_foreground(unsigned int) = 0;
    virtual void set_background(unsigned int) = 0;

    virtual void draw_pixel(int, int) = 0;
    virtual void draw_pixels(GRmultiPt*, int) = 0;

    virtual void set_linestyle(const GRlineType*) = 0;
    virtual void draw_line(int, int, int, int) = 0;
    virtual void draw_polyline(GRmultiPt*, int) = 0;
    virtual void draw_lines(GRmultiPt*, int) = 0;

    virtual void define_fillpattern(GRfillType*) = 0;
    virtual void set_fillpattern(const GRfillType*) = 0;
    virtual void draw_box(int, int, int, int) = 0;
    virtual void draw_boxes(GRmultiPt*, int) = 0;
    virtual void draw_arc(int, int, int, int, double, double) = 0;
    virtual void draw_polygon(GRmultiPt*, int) = 0;
    virtual void draw_zoid(int, int, int, int, int, int) = 0;
    virtual void draw_image(const GRimage*, int, int, int, int) = 0;

    virtual void set_font(QFont*) = 0;
    virtual int text_width(QFont*, const char*, int) = 0;
    virtual void text_extent(const char*, int*, int*) = 0;
    virtual void draw_text(int, int, const char*, int) = 0;
    virtual void draw_glyph(int, int, const unsigned char*, int) = 0;

    virtual void draw_pixmap(int, int, QPixmap*, int, int, int, int) = 0;
    virtual void draw_image(int, int, QImage*, int, int, int, int) = 0;

    // Ghost drawing.
    virtual void set_ghost_common(cGhostDrawCommon*) = 0;
    virtual void set_ghost(GhostDrawFunc, int, int) = 0;
    virtual void show_ghost(bool) = 0;
    virtual void undraw_ghost(bool) = 0;
    virtual void draw_ghost(int, int) = 0;

    virtual void set_ghost_mode(bool) = 0;
    virtual void set_ghost_color(unsigned int) = 0;
    virtual bool has_ghost() = 0;
    virtual bool showing_ghost() = 0;
    virtual GRlineDb *linedb() = 0;
    virtual GhostDrawFunc get_ghost_func() = 0;
    virtual void set_ghost_func(GhostDrawFunc) = 0;
};

