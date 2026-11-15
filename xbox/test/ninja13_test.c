/* Ninja's bodies whose make gcc side is GCC's own code, against xbox/sdk/ninja/:
 * the matrix stack's njPushMatrix and njPopMatrix, njTranslate,
 * njTranslateV, njScale, njScaleV, njGetTranslation and njCalcPoint, of a
 * matrix and of XMTRX, and njInnerProduct, njUnitVector, njScalor,
 * njScalor2, njSqrt, njInvertSqrt, njSin, njCos and njTan. the SH-4 run
 * takes the original bodies as all.sh splits them from SHC's side */
#include "harness.h"

typedef float Matrix[16];
typedef struct {
    float x, y, z;
} Vector;

/* njGlobal.c's, which the harness defines on both runs */
int _stack_size_;
int _max_stack_size_;
Matrix *_nj_current_matrix_ptr_;

int njPushMatrix(Matrix *m);
int njPopMatrix(unsigned n);
void njTranslate(Matrix *m, float x, float y, float z);
void njTranslateV(Matrix *m, Vector *v);
void njScale(Matrix *m, float sx, float sy, float sz);
void njScaleV(Matrix *m, Vector *v);
void njGetTranslation(Matrix *m, Vector *p);
void njCalcPoint(Matrix *m, Vector *ps, Vector *pd);
float njInnerProduct(Vector *v1, Vector *v2);
float njUnitVector(Vector *v);
float njScalor(Vector *v);
float njScalor2(Vector *v);
float njSqrt(float n);
float njInvertSqrt(float n);
float njSin(int n);
float njCos(int n);
float njTan(int n);

static void put(const float *m, int count)
{
    int k;

    for (k = 0; k < count; k++) {
        harness_float(m[k]);
    }
}

static void put_xmtrx(void)
{
    Matrix x;

    harness_store_xmtrx(x);
    put(x, 16);
}

static Vector vector(float range)
{
    Vector v;

    v.x = harness_value(range);
    v.y = harness_value(range);
    v.z = harness_value(range);
    return v;
}

int main(void)
{
    static Matrix stack[6];
    unsigned i;

    harness_fpu();
    for (i = 0; i < 2000; i++) {
        Matrix m;
        Matrix x;
        Vector v = vector(300.0F);
        Vector w = vector(300.0F);
        Vector out;
        int k;

        for (k = 0; k < 16; k++) {
            m[k] = harness_value(3.0F);
            x[k] = harness_value(3.0F);
        }

        /* the stack: four pushes into room for four more, the last refused,
         * then pops of one, two and past the depth */
        for (k = 0; k < 16; k++) {
            stack[0][k] = 0.0F;
        }
        _nj_current_matrix_ptr_ = &stack[0];
        _stack_size_ = 1;
        _max_stack_size_ = 4;
        harness_load_xmtrx(x);
        harness_begin("stack", i);
        harness_hex((unsigned)njPushMatrix(&m));
        put_xmtrx();
        harness_hex((unsigned)njPushMatrix(0));
        harness_hex((unsigned)njPushMatrix(&m));
        harness_hex((unsigned)njPushMatrix(0));
        harness_hex((unsigned)_stack_size_);
        harness_hex((unsigned)(_nj_current_matrix_ptr_ - stack));
        for (k = 0; k < 4; k++) {
            put(stack[k], 16);
        }
        harness_hex((unsigned)njPopMatrix(1));
        put_xmtrx();
        harness_hex((unsigned)njPopMatrix(2));
        put_xmtrx();
        harness_hex((unsigned)njPopMatrix(1));
        harness_hex((unsigned)_stack_size_);
        harness_hex((unsigned)(_nj_current_matrix_ptr_ - stack));
        harness_end();

        harness_begin("translate", i);
        {
            Matrix a;

            for (k = 0; k < 16; k++) {
                a[k] = m[k];
            }
            njTranslate(&a, v.x, v.y, v.z);
            put(a, 16);
            njTranslateV(&a, &w);
            put(a, 16);
            harness_load_xmtrx(x);
            njTranslate(0, v.x, v.y, v.z);
            put_xmtrx();
            njTranslateV(0, &w);
            put_xmtrx();
        }
        harness_end();

        harness_begin("scale", i);
        {
            Matrix a;
            Vector s = vector(4.0F);

            for (k = 0; k < 16; k++) {
                a[k] = m[k];
            }
            njScale(&a, s.x, s.y, s.z);
            put(a, 16);
            njScaleV(&a, &s);
            put(a, 16);
            harness_load_xmtrx(x);
            njScale(0, s.x, s.y, s.z);
            put_xmtrx();
            njScaleV(0, &s);
            put_xmtrx();
        }
        harness_end();

        harness_load_xmtrx(x);
        harness_begin("point", i);
        njGetTranslation(&m, &out);
        put(&out.x, 3);
        njGetTranslation(0, &out);
        put(&out.x, 3);
        njCalcPoint(&m, &v, &out);
        put(&out.x, 3);
        njCalcPoint(0, &v, &out);
        put(&out.x, 3);
        out = v;
        njCalcPoint(&m, &out, &out);
        put(&out.x, 3);
        out = v;
        njCalcPoint(0, &out, &out);
        put(&out.x, 3);
        harness_end();

        harness_begin("vector", i);
        harness_float(njInnerProduct(&v, &w));
        harness_float(njScalor(&v));
        harness_float(njScalor2(&v));
        out = v;
        harness_float(njUnitVector(&out));
        put(&out.x, 3);
        out.x = out.y = out.z = 0.0F;
        harness_float(njUnitVector(&out));
        put(&out.x, 3);
        harness_end();

        harness_begin("scalar", i);
        {
            float n = harness_value(1000.0F);
            int ang = (int)harness_random();

            harness_float(njSqrt(n));
            harness_float(njSqrt(-n));
            harness_float(njInvertSqrt(n));
            harness_float(njInvertSqrt(-n));
            harness_float(njSqrt(0.0F));
            harness_float(njSqrt(-0.0F));
            harness_float(njSin(ang));
            harness_float(njCos(ang));
            harness_float(njTan(ang));
            harness_float(njSin(ang & 0xc000));
            harness_float(njCos(ang & 0xc000));
        }
        harness_end();
    }
    return 0;
}
