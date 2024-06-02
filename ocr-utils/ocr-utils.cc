// -*- C++ -*-

// Copyright 2006-2007 Deutsches Forschungszentrum fuer Kuenstliche Intelligenz
// or its licensors, as applicable.
//
// You may not use this file except under the terms of the accompanying license.
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You may
// obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Project:
// File: ocr-utils.cc
// Purpose: miscelaneous routines
// Responsible: mezhirov
// Reviewer:
// Primary Repository:
// Web Sites: www.iupr.org, www.dfki.de

#include <stdarg.h>
#include "ocropus.h"
#include "colib/narray-ops.h"

using namespace iulib;
using namespace colib;

namespace ocropus {
    param_bool bgcheck("bgcheck", false, "abort on detecting an inverted image");

    void invert(bytearray &a) {
        int n = a.length1d();
        for (int i = 0; i < n; i++) {
            a.at1d(i) = 255 - a.at1d(i);
        }
    }

    void crop_masked(bytearray &result,
                     bytearray &source,
                     rectangle crop_rect,
                     bytearray &mask,
                     int def_val,
                     int pad) {
        CHECK_ARG(background_seems_black(mask));

        rectangle box(0, 0, source.dim(0), source.dim(1));
        box.intersect(crop_rect);
        result.resize(box.width() + 2 * pad, box.height() + 2 * pad);
        fill(result, def_val);
        for(int x = 0; x < box.width(); x++) {
            for(int y = 0; y < box.height(); y++) {
               if(mask(x + box.x0, y + box.y0))
                   result(x + pad, y + pad) = source(x + box.x0, y + box.y0);
            }
        }
    }


    int average_on_border(colib::bytearray &a) {
        int sum = 0;
        int right = a.dim(0) - 1;
        int top = a.dim(1) - 1;
        for(int x = 0; x < a.dim(0); x++)
            sum += a(x, 0);
        for(int x = 0; x < a.dim(0); x++)
            sum += a(x, top);
        for(int y = 1; y < top; y++)
            sum += a(0, y);
        for(int y = 1; y < top; y++)
            sum += a(right, y);
        // If average border intensity is between 127-128, inverting the
        // image does not work correctly
        float average_border_intensity = sum / ((right + top) * 2.0);
        ASSERTWARN(average_border_intensity<=127 || average_border_intensity>=128);
        return sum / ((right + top) * 2);
    }


    // FIXME/mezhirov use imgmorph stuff --tmb

    void blit2d(bytearray &dest, const bytearray &src, int shift_x, int shift_y) {
        int w = src.dim(0);
        int h = src.dim(1);
        for (int x=0;x<w;x++) for (int y=0;y<h;y++) {
            dest(x + shift_x, y + shift_y) = src(x,y);
        }
    }

    float median(intarray &a) {
        intarray s;
        copy(s, a);
        quicksort(s);
        int n = s.length();
        if (!n)
            return 0;
        if (n % 2)
            return s[n / 2];
        else
            return float(s[n / 2 - 1] + s[n / 2]) / 2;
    }



    void plot_hist(FILE *stream, floatarray &hist){
        if(!stream){
            fprintf(stderr,"Unable to open histogram image stream.\n");
            exit(0);
        }
        int maxval = 1000;
        int len    = hist.length();
        narray<unsigned char> image(len, maxval);
        fill(image,0xff);
        for(int x=0; x<len; x++){
            int top = min(maxval-1,int(hist[x]));
            for(int y=0; y<top; y++)
                image(x,y) = 0;
        }
        write_png(stream,image);
        fclose(stream);
    }

    void paint_box(intarray &image, rectangle r, int color, bool inverted){

        int width  = image.dim(0);
        int height = image.dim(1);
        int left, top, right, bottom;

        left   = (r.x0<0)  ? 0   : r.x0;
        top    = (r.y0<0)  ? 0   : r.y0;
        right  = (r.x1>=width) ? width-1 : r.x1;
        bottom = (r.y1>=height) ? height-1 : r.y1;

        if(right <= left || bottom <= top) return;

        for(int x=left; x<right; x++){
            for(int y=top; y<bottom; y++){
                if(!inverted)
                    image(x,y) &= color;
                else
                    image(x,y) |= color;
            }
        }
    }

    void paint_box(bytearray &image, rectangle r, colib::byte color, bool inverted){

        int width  = image.dim(0);
        int height = image.dim(1);
        int left, top, right, bottom;

        left   = (r.x0<0)  ? 0   : r.x0;
        top    = (r.y0<0)  ? 0   : r.y0;
        right  = (r.x1>=width) ? width-1 : r.x1;
        bottom = (r.y1>=height) ? height-1 : r.y1;

        if(right <= left || bottom <= top) return;

        for(int x=left; x<right; x++){
            for(int y=top; y<bottom; y++){
                if(!inverted)
                    image(x,y) &= color;
                else
                    image(x,y) |= color;
            }
        }
    }

    void paint_box_border(intarray &image, rectangle r, int color, bool inverted){

        int width  = image.dim(0);
        int height = image.dim(1);
        int left, top, right, bottom;

        left   = (r.x0<0)  ? 0   : r.x0;
        top    = (r.y0<0)  ? 0   : r.y0;
        right  = (r.x1>=width) ? width-1 : r.x1;
        bottom = (r.y1>=height) ? height-1 : r.y1;
        if(right < left || bottom < top) return;
        int x,y;
        if(!inverted){
            for(x=left; x<=right; x++){ image(x,top)     &=color; }
            for(x=left; x<=right; x++){ image(x,bottom)  &=color; }
            for(y=top; y<=bottom; y++){ image(left,y)    &=color; }
            for(y=top; y<=bottom; y++){ image(right,y)   &=color; }
        }else{
            for(x=left; x<=right; x++){ image(x,top)     |=color; }
            for(x=left; x<=right; x++){ image(x,bottom)  |=color; }
            for(y=top; y<=bottom; y++){ image(left,y)    |=color; }
            for(y=top; y<=bottom; y++){ image(right,y)   |=color; }
        }
    }

    void paint_box_border(bytearray &image, rectangle r, colib::byte color, bool inverted){

        int width  = image.dim(0);
        int height = image.dim(1);
        int left, top, right, bottom;

        left   = (r.x0<0)  ? 0   : r.x0;
        top    = (r.y0<0)  ? 0   : r.y0;
        right  = (r.x1>=width) ? width-1 : r.x1;
        bottom = (r.y1>=height) ? height-1 : r.y1;
        if(right < left || bottom < top) return;
        int x,y;
        if(!inverted){
            for(x=left; x<=right; x++){ image(x,top)     &=color; }
            for(x=left; x<=right; x++){ image(x,bottom)  &=color; }
            for(y=top; y<=bottom; y++){ image(left,y)    &=color; }
            for(y=top; y<=bottom; y++){ image(right,y)   &=color; }
        }else{
            for(x=left; x<=right; x++){ image(x,top)     |=color; }
            for(x=left; x<=right; x++){ image(x,bottom)  |=color; }
            for(y=top; y<=bottom; y++){ image(left,y)    |=color; }
            for(y=top; y<=bottom; y++){ image(right,y)   |=color; }
        }
    }

    static void subsample_boxes(narray<rectangle> &boxes, int factor) {
        int len = boxes.length();
        if (factor == 0) return;
        for(int i=0; i<len; i++){
            boxes[i].x0 = boxes[i].x0/factor;
            boxes[i].x1 = boxes[i].x1/factor;
            boxes[i].y0 = boxes[i].y0/factor;
            boxes[i].y1 = boxes[i].y1/factor;
        }
    }


    void draw_rects(colib::intarray &out, colib::bytearray &in,
                    colib::narray<colib::rectangle> &rects,
                    int downsample_factor,  int color){
        int ds = downsample_factor;
        if(ds <= 0)
            ds = 1;
        int width  = in.dim(0);
        int height = in.dim(1);
        int xdim   = width/ds;
        int ydim   = height/ds;
        out.resize(xdim, ydim);
        for(int ix=0; ix<xdim; ix++)
            out(ix,ydim-1)=0x00ffffff;
        for(int x=0,ix=0; x<width-ds; x+=ds, ix++) {
            for(int y=0,iy=0; y<height-ds; y+=ds, iy++){
                out(ix,iy)=in(x,y)*0x00010101;
            }
        }
        narray<rectangle> boxes;
        copy(boxes,rects);
        if(ds > 1)
            subsample_boxes(boxes, ds);

        for(int i=0, len=boxes.length(); i<len; i++)
            paint_box_border(out, boxes[i], color);

    }

    void draw_filled_rects(colib::intarray &out, colib::bytearray &in,
                           colib::narray<colib::rectangle> &rects,
                           int downsample_factor, int color, int border_color){
        int ds = downsample_factor;
        if(ds <= 0)
            ds = 1;
        int width  = in.dim(0);
        int height = in.dim(1);
        int xdim   = width/ds;
        int ydim   = height/ds;
        out.resize(xdim, ydim);
        for(int ix=0; ix<xdim; ix++)
            out(ix,ydim-1)=0x00ffffff;
        for(int x=0,ix=0; x<width-ds; x+=ds, ix++) {
            for(int y=0,iy=0; y<height-ds; y+=ds, iy++){
                out(ix,iy)=in(x,y)*0x00010101;
            }
        }
        narray<rectangle> boxes;
        copy(boxes,rects);
        if(ds > 1)
            subsample_boxes(boxes, ds);

        for(int i=0, len=boxes.length(); i<len; i++){
            paint_box(out, boxes[i], color);
            paint_box_border(out, boxes[i], border_color);
        }

    }

    // FIXME/mezhirov add comments --tmb

    void get_line_info(float &baseline, float &xheight, float &descender, float &ascender, intarray &seg) {
        narray<rectangle> bboxes;
        bounding_boxes(bboxes, seg);

        intarray tops, bottoms;
        makelike(tops,    bboxes);
        makelike(bottoms, bboxes);

        for(int i = 0; i < bboxes.length(); i++) {
            tops[i] = bboxes[i].y1;
            bottoms[i] = bboxes[i].y0;
        }

        baseline = median(bottoms) + 1;
        xheight = median(tops) - baseline;

        descender = baseline - 0.4 * xheight;
        ascender  = baseline + 2 * xheight;
    }

    // FIXME/mezhirov add comments --tmb

    static const char *version_string = NULL;

    // FIXME/mezhirov add comments --tmb

    const char *get_version_string() {
        return version_string;
    }

    // FIXME/mezhirov add comments --tmb

    void set_version_string(const char *new_version_string) {
        if (version_string) {
            ASSERT(new_version_string && !strcmp(version_string, new_version_string));
        } else {
            version_string = new_version_string;
        }
    }

    void align_segmentation(intarray &segmentation,narray<rectangle> &bboxes) {
        intarray temp;
        make_line_segmentation_black(segmentation);
        renumber_labels(segmentation,1);
        int nsegs = max(segmentation)+1;
        intarray counts;
        counts.resize(nsegs,bboxes.length());
        fill(counts,0);
        for(int i=0;i<segmentation.dim(0);i++) {
            for(int j=0;j<segmentation.dim(1);j++) {
                int cs = segmentation(i,j);
                if(cs==0) continue;
                for(int k=0;k<bboxes.length();k++) {
                    if(bboxes[k].contains(i,j))
                        counts(cs,k)++;
                }
            }
        }
        intarray segmap;
        segmap.resize(counts.dim(0));
        for(int i=0;i<counts.dim(0);i++) {
            int mj = -1;
            int mc = 0;
            for(int j=0;j<counts.dim(1);j++) {
                if(counts(i,j)>mc) {
                    mj = j;
                    mc = counts(i,j);
                }
            }
            segmap(i) = mj;
        }
        for(int i=0;i<segmentation.dim(0);i++) {
            for(int j=0;j<segmentation.dim(1);j++) {
                int cs = segmentation(i,j);
                if(cs) continue;
                segmentation(i,j) = segmap(cs)+1;
            }
        }
    }

    namespace {
        void getrow(intarray &a,intarray &m,int i) {
            a.resize(m.dim(1));
            for(int j=0;j<m.dim(1);j++) a(j) = m(i,j);
        }
        void getcol(intarray &a,intarray &m,int j) {
            a.resize(m.dim(0));
            for(int i=0;i<m.dim(0);i++) a(i) = m(i,j);
        }
    }

    void evaluate_segmentation(int &nover,int &nunder,int &nmis,intarray &model_raw,intarray &image_raw,float tolerance) {
        CHECK_ARG(samedims(model_raw,image_raw));

        intarray model,image;
        copy(model,model_raw);
        replace_values(model, 0xFFFFFF, 0);
        int nmodel = renumber_labels(model,1);
        CHECK_ARG(nmodel<100000);

        copy(image,image_raw);
        replace_values(image, 0xFFFFFF, 0);
        int nimage = renumber_labels(image,1);
        CHECK_ARG(nimage<100000);

        intarray table(nmodel,nimage);
        fill(table,0);
        for(int i=0;i<model.length1d();i++)
            table(model.at1d(i),image.at1d(i))++;

//         for(int i=1;i<table.dim(0);i++) {
//             for(int j=1;j<table.dim(1);j++) {
//                 printf(" %3d",table(i,j));
//             }
//             printf("\n");
//         }

        nover = 0;
        nunder = 0;
        nmis = 0;

        for(int i=1;i<table.dim(0);i++) {
            intarray row;
            getrow(row,table,i);
            row(0) = 0;
            double total = sum(row);
            int match = argmax(row);
            // printf("[%3d,] %3d: ",i,match); for(int j=1;j<table.dim(1);j++) printf(" %3d",table(i,j)); printf("\n");
            for(int j=1;j<table.dim(1);j++) {
                if(j==match) continue;
                int count = table(i,j);
                if(count==0) continue;
                if(count / total > tolerance) {
                    nover++;
                } else {
                    nmis++;
                }
            }
        }
        for(int j=1;j<table.dim(1);j++) {
            intarray col;
            getcol(col,table,j);
            col(0) = 0;
            double total = sum(col);
            int match = argmax(col);
            // printf("[,%3d] %3d: ",j,match); for(int i=1;i<table.dim(0);i++) printf(" %3d",table(i,j)); printf("\n");
            for(int i=1;i<table.dim(0);i++) {
                if(i==match) continue;
                int count = table(i,j);
                if(count==0) continue;
                if(count / total > tolerance) {
                    nunder++;
                } else {
                    nmis++;
                }
            }
        }
    }
    void ocr_bboxes_to_charseg(intarray &cseg,narray<rectangle> &bboxes,intarray &segmentation) {
        make_line_segmentation_black(segmentation);
        CHECK_ARG(max(segmentation)<100000);
        intarray counts(max(segmentation)+1,bboxes.length());
        fill(counts,0);
        for(int i=0;i<segmentation.dim(0);i++) for(int j=0;j<segmentation.dim(1);j++) {
            int value = segmentation(i,j);
            if(value==0) continue;
            for(int k=0;k<bboxes.length();k++) {
                rectangle bbox = bboxes[k];
                if(bbox.includes(i,j))
                    counts(value,k)++;
            }
        }
        intarray valuemap(max(segmentation)+1);
        fill(valuemap,0);
        for(int i=1;i<counts.dim(0);i++)
            valuemap(i) = rowargmax(counts,i)+1;
        makelike(cseg,segmentation);
        for(int i=0;i<segmentation.dim(0);i++) for(int j=0;j<segmentation.dim(1);j++) {
            cseg(i,j) = valuemap(segmentation(i,j));
        }
    }

    template <class T>
    void remove_small_components(narray<T> &bimage,int mw,int mh) {
        intarray image;
        copy(image,bimage);
        label_components(image);
        narray<rectangle> rects;
        bounding_boxes(rects,image);
        if(rects.length()==0) return;
        bytearray good(rects.length());
        for(int i=0;i<good.length();i++)
            good[i] = 1;
        for(int i=0;i<rects.length();i++)
            if(rects[i].width()<mw && rects[i].height()<mh)
                good[i] = 0;
        for(int i=0;i<image.length();i++)
            if(!good(image[i]))
                image[i] = 0;
        for(int i=0;i<image.length1d();i++)
            if(!image[i])
                bimage[i] = 0;
    }
    template void remove_small_components<colib::byte>(narray<colib::byte> &,int,int);
    template void remove_small_components<int>(narray<int> &,int,int);

    template <class T>
    void remove_marginal_components(narray<T> &bimage,int x0,int y0,int x1,int y1) {
        intarray image;
        copy(image,bimage);
        label_components(image);
        narray<rectangle> rects;
        bounding_boxes(rects,image);
        if(rects.length()>0) {
            x1 = bimage.dim(0)-x1;
            y1 = bimage.dim(1)-y1;
            bytearray good(rects.length());
            fill(good, 1);
            for(int i=0;i<rects.length();i++) {
                rectangle r = rects[i];
                // >= ?
                if(r.x1 < x0 || r.x0 > x1 || r.y1 < y0 || r.y0 > y1) {
                    good[i] = 0;
                }
            }
            for(int i=0;i<image.length1d();i++) {
                if(!good(image.at1d(i)))
                    bimage.at1d(i) = 0;
            }
        }
    }
    template void remove_marginal_components<colib::byte>(narray<colib::byte> &,int,int,int,int);
    template void remove_marginal_components<int>(narray<int> &,int,int,int,int);

    void remove_neighbour_line_components(bytearray &line) {
        invert(line);
        intarray image;
        copy(image,line);
        label_components(image);
        narray<rectangle> rects;
        bounding_boxes(rects,image);
        if(rects.length()>0) {
            int h = line.dim(1);
            int lower = int(h*0.33);
            int upper = int(h*0.67);
            bytearray good(rects.length());
            fill(good, 1);
            for(int i=0;i<rects.length();i++) {
                rectangle r = rects[i];
                if( ((r.y0 == 0) && (r.y1 < lower)) ||
                    ((r.y1 == h) && (r.y0 > upper)) )
                    good[i] = 0;
            }
            for(int i=0;i<image.length1d();i++)
                if(!good(image.at1d(i)))
                    line.at1d(i) = 0;
        }
        invert(line);
    }

    /// Analogous to python's split().
    void split_string(narray<strg> &components,
                      const char *s,
                      const char *delimiters) {
        components.clear();
        if(!*s) return;
        while(1) {
            const char *p = s;
            while(*p && !strchr(delimiters, *p))
                p++;
            int len = p - s;
            if(len) {
                components.push().append(s, len);
            }
            if(!*p) return;
            s = p + 1;
        }
    }

    int binarize_simple(bytearray &result, bytearray &image) {
        int threshold = (max(image)+min(image))/2;
        makelike(result,image);
        for(int i=0;i<image.length1d();i++)
            result.at1d(i) = image.at1d(i)<threshold ? 0 : 255;
        return threshold;
    }

    int binarize_simple(colib::bytearray &image) {
        return binarize_simple(image, image);
    }

    void binarize_with_threshold(floatarray &out, floatarray &in, float threshold) {
        makelike(out,in);
        for(int i=0;i<in.length1d();i++)
            out.at1d(i) = in.at1d(i)<threshold ? 0.0 : 1.0;
    }

    void binarize_with_threshold(bytearray &out, floatarray &in, float threshold) {
        makelike(out,in);
        for(int i=0;i<in.length1d();i++)
            out.at1d(i) = in.at1d(i)<threshold ? 0 : 255;
    }

    void binarize_with_threshold(bytearray &out, bytearray &in, int threshold) {
        makelike(out,in);
        for(int i=0;i<in.length1d();i++)
            out.at1d(i) = in.at1d(i)<threshold ? 0 : 255;
    }

    void binarize_with_threshold(floatarray &in, float threshold) {
        floatarray out;
        binarize_with_threshold(out, in, threshold);
        in.move(out);
    }

    void binarize_with_threshold(bytearray &in, int threshold) {
        bytearray out;
        binarize_with_threshold(out, in, threshold);
        in.move(out);
    }

    void runlength_histogram(floatarray &hist,
                             bytearray &img,
                             rectangle box,
                             bool white,
                             bool vertical){
        fill(hist,0);
        white = !!white;
        if(!vertical) {
            for(int j=box.y0;j<box.y1;j++){
                int flag = 0;
                int runlength = 0;
                for(int k=box.x0;k<box.x1;k++) {
                    if (!!img(k,j)==white) {
                        runlength++;
                        flag = 1;
                    }
                    if (!!img(k,j) != white && flag) {
                        flag = 0;
                        if(runlength < hist.length())
                            hist(runlength)++;
                        runlength = 0;
                    }
                }
                if (flag && runlength < hist.length())
                    hist(runlength)++;
            }
        } else {
            for(int k=box.x0;k<box.x1;k++) {
                int flag = 0;
                int runlength = 0;
                for(int j=box.y0;j<box.y1;j++){
                    if (!!img(k,j)==white) {
                        runlength++;
                        flag = 1;
                    }
                    if (!!img(k,j) != white && flag) {
                        flag = 0;
                        if(runlength < hist.length())
                            hist(runlength)++;
                        runlength = 0;
                    }
                }
                if (flag && runlength < hist.length())
                    hist(runlength)++;
            }
        }
    }

    /// Return the index of the histogram median.
    int find_median_in_histogram(narray<float> &hist){
        int index= 0;
        float partial_sum = 0, sum = 0 ;
        for(int i = 0; i < hist.length(); i++) sum += hist(i);
        for(int j = 0; j < hist.length(); j++) hist(j) /= sum;
        while(partial_sum < 0.5) {
            partial_sum += hist(index);
            index++;
        }
        return index;
    }

    void throw_fmt(const char *format, ...) {
        va_list v;
        va_start(v, format);
        static char buf[1000];
        vsnprintf(buf, sizeof(buf), format, v);
        va_end(v);
        throw (const char *) buf;
    }

    void optional_check_background_is_darker(colib::bytearray &a) {
        if(bgcheck) {
            CHECK_CONDITION2(average_on_border(a) <= (min(a) + max(a) / 2),
                             "some image failed an internal sanity check; disable this check with bgcheck=0");

        }
    }
    void optional_check_background_is_lighter(colib::bytearray &a) {
        if(bgcheck) {
            CHECK_CONDITION2(average_on_border(a) >= (min(a) + max(a) / 2),
                             "some image failed an internal sanity check; disable this check with bgcheck=0");
        }
    }

    void paint_rectangles(intarray &image,rectarray &rectangles) {
        int w = image.dim(0), h = image.dim(1);
        for(int i=0;i<rectangles.length();i++) {
            rectangle r = rectangles[i];
            r.x0 = r.x0<0?0:r.x0;
            r.y0 = r.y0<0?0:r.y0;
            r.x1 = r.x1>=w?w-1:r.x1;
            r.y1 = r.y1>=h?h-1:r.y1;
            int color = i+1;
            for(int x=r.x0;x<r.x1;x++)
                for(int y=r.y0;y<r.y1;y++)
                    image(x,y) = color;
        }
    }

    template<class T>
    void rotate_90(narray<T> &out, narray<T> &in) {
        out.resize(in.dim(1),in.dim(0));
        for (int x=0;x<in.dim(0);x++)
            for (int y=0;y<in.dim(1);y++)
                out(y,in.dim(0)-x-1) = in(x,y);
    }
    template void rotate_90<colib::byte>(narray<colib::byte> &,narray<colib::byte> &);
    template void rotate_90<int>(narray<int> &,narray<int> &);
    template void rotate_90<float>(narray<float> &,narray<float> &);


    template<class T>
    void rotate_270(narray<T> &out, narray<T> &in) {
        out.resize(in.dim(1), in.dim(0));
        for (int x=0;x<in.dim(0);x++) {
            for (int y=0; y<in.dim(1);y++)
                out(in.dim(1)-y-1,x) = in(x,y);
        }
    }
    template void rotate_270<colib::byte>(narray<colib::byte> &,narray<colib::byte> &);
    template void rotate_270<int>(narray<int> &,narray<int> &);
    template void rotate_270<float>(narray<float> &,narray<float> &);

    template<class T>
    void rotate_180(narray<T> &out, narray<T> &in) {
        out.resize(in.dim(0), in.dim(1));
        for (int x=0;x<in.dim(0);x++) {
            for (int y=0; y<in.dim(1);y++)
                out(in.dim(0)-x-1,in.dim(1)-y-1) = in(x,y);
        }
    }
    template void rotate_180<colib::byte>(narray<colib::byte> &,narray<colib::byte> &);
    template void rotate_180<int>(narray<int> &,narray<int> &);
    template void rotate_180<float>(narray<float> &,narray<float> &);

    float estimate_linesize(bytearray &image,float f,float minsize) {
        floatarray sizes;
        for(int i=0;i<image.dim(0);i++) {
            int lo=-1,hi=-1;
            for(int j=0;j<image.dim(1);j++) {
                if(image(i,j)>=128) continue;
                if(lo<0) lo = j;
                hi = j;
            }
            if(hi==lo) continue;
            if(hi-lo<minsize) continue;
            sizes.push(hi-lo);
        }
        if(sizes.length()<1) return 0;
        float v = fractile(sizes,f);
        return v;
    }

    float estimate_strokewidth(bytearray &image,float f) {
        int w = image.dim(0);
        int h = image.dim(1);
        floatarray widths;
        for(int i=0;i<w;i++) {
            int j = 0;
            while(j<h) {
                while(j<h && image(i,j)>=128) j++;
                int start = j;
                if(j>=h) break;
                while(j<h && image(i,j)<128) j++;
                int d = j-start;
                widths.push(d);
            }
        }
        for(int j=0;j<h;j++) {
            int i = 0;
            while(i<w) {
                while(i<w && image(i,j)>=128) i++;
                int start = i;
                if(i>=w) break;
                while(i<w && image(i,j)<128) i++;
                int d = i-start;
                widths.push(d);
            }
        }
        if(widths.length()<1) return 0;
        float v = fractile(widths,f);
        return v;
    }

    float estimate_size_by_box(bytearray &image,float f) {
        intarray labels;
        labels = image;
        label_components(labels);
        narray<rectangle> rects;
        bounding_boxes(rects,labels);
        floatarray sizes;
        for(int i=1;i<rects.length();i++) {
            int d = rects[i].y1-rects[i].y0;
            debugf("ds","%d\n",d);
            sizes.push(d);
        }
        float v = sum(sizes)*1.0/sizes.length();
        return v;
    }

    int count_neighbors(bytearray &bi,int x,int y) {
        int nn=0;
        if (bi(x+1,y)) nn++;
        if (bi(x+1,y+1)) nn++;
        if (bi(x,y+1)) nn++;
        if (bi(x-1,y+1)) nn++;
        if (bi(x-1,y)) nn++;
        if (bi(x-1,y-1)) nn++;
        if (bi(x,y-1)) nn++;
        if (bi(x+1,y-1)) nn++;
        return nn;
    }

    void skeletal_features(bytearray &endpoints,
                           bytearray &junctions,
                           bytearray &image,
                           float presmooth,
                           float skelsmooth) {
        using namespace narray_ops;
        bytearray temp;
        temp.copy(image);
        greater(temp,128,0,255);
        if(presmooth>0) {
            gauss2d(temp,presmooth,presmooth);
            greater(temp,128,0,255);
        }
        thin(temp);
        dshow(temp,"yYy");
        makelike(junctions,temp);
        junctions = 0;
        makelike(endpoints,temp);
        endpoints = 0;
        for(int i=1;i<temp.dim(0)-1;i++) {
            for(int j=1;j<temp.dim(1)-1;j++) {
                if(!temp(i,j)) continue;
                int n = count_neighbors(temp,i,j);
                if(n==1) endpoints(i,j) = 255;
                if(n>2) junctions(i,j) = 255;
            }
        }
        binary_dilate_circle(junctions,int(skelsmooth+0.5));
        binary_dilate_circle(endpoints,int(skelsmooth+0.5));
    }

    void skeletal_feature_counts(int &nendpoints,
                                 int &njunctions,
                                 bytearray &image,
                                 float presmooth,
                                 float skelsmooth) {
        bytearray endpoints;
        bytearray junctions;
        ocropus::skeletal_features(endpoints,junctions,image,presmooth,skelsmooth);
        intarray temp;
        binary_dilate_circle(endpoints,1);
        binary_dilate_circle(junctions,1);
        temp = endpoints;
        label_components(temp);
        nendpoints = max(temp);
        temp = junctions;
        label_components(temp);
        njunctions = max(temp);
    }

    int endpoints_counts(bytearray &image,float presmooth,float skelsmooth) {
        int nendpoints,njunctions;
        skeletal_feature_counts(nendpoints,njunctions,image,presmooth,skelsmooth);
        return nendpoints;
    }

    int junction_counts(bytearray &image,float presmooth,float skelsmooth) {
        int nendpoints,njunctions;
        skeletal_feature_counts(nendpoints,njunctions,image,presmooth,skelsmooth);
        return njunctions;
    }

    int component_counts(bytearray &image,float presmooth) {
        using namespace narray_ops;
        bytearray temp;
        temp.copy(image);
        greater(temp,128,0,255);
        if(presmooth>0) {
            gauss2d(temp,presmooth,presmooth);
            greater(temp,128,0,255);
        }
        intarray blobs;
        blobs.copy(temp);
        label_components(blobs);
        return max(blobs);
    }

    int hole_counts(bytearray &image,float presmooth) {
        using namespace narray_ops;
        bytearray temp;
        temp.copy(image);
        greater(temp,128,0,255);
        if(presmooth>0) {
            gauss2d(temp,presmooth,presmooth);
            greater(temp,128,0,255);
        }
        sub(255,temp);
        intarray blobs;
        blobs.copy(temp);
        label_components(blobs);
        return max(blobs)-1;
    }

    template <class T>
    void copy_rect(narray<T> &dst,int x,int y,narray<T> &src,int x0,int y0,int x1,int y1) {
        CHECK_ARG(dst.rank()==src.rank());
        int dw = dst.dim(0), dh = dst.dim(1);
        int sw = src.dim(0), sh = src.dim(1);
        int w = min(min(sw-x0,x1-x0),dw-x);
        int h = min(min(sh-y0,y1-y0),dh-y);
        if(src.rank()==2) {
            for(int i=0;i<w;i++) for(int j=0;j<h;j++) {
                dst(i+x,j+y) = src(i+x0,j+y0);
            }
        } else if(src.rank()==3) {
            CHECK_ARG(dst.dim(2)==src.dim(2));
            for(int i=0;i<w;i++) for(int j=0;j<h;j++) {
                for(int k=0;k<src.dim(2);k++)
                    dst(i+x,j+y,k) = src(i+x0,j+y0,k);
            }
        } else {
            throw "bad rank";
        }
    }
    template void copy_rect<colib::byte>(narray<colib::byte> &,int,int,narray<colib::byte> &,int,int,int,int);
    template void copy_rect<int>(narray<int> &,int,int,narray<int> &,int,int,int,int);
    template void copy_rect<float>(narray<float> &,int,int,narray<float> &,int,int,int,int);

    inline float normorient(float x) {
        while(x>=M_PI/2) x -= M_PI;
        while(x<-M_PI/2) x += M_PI;
        return x;
    }
    inline float normadiff(float x) {
        while(x>=M_PI) x -= 2*M_PI;
        while(x<-M_PI) x += 2*M_PI;
        return x;
    }
    inline float normorientplus(float x) {
        while(x>=M_PI) x -= M_PI;
        while(x<0) x += M_PI;
        return x;
    }

    inline void checknan(floatarray &v) {
        int n = v.length1d();
        for(int i=0;i<n;i++)
            CHECK(!isnan(v.unsafe_at1d(i)));
    }

    inline float floordiv(float x,float y) {
        float n = floor(x/y);
        return x - n*y;
    }

    void ridgemap(narray<floatarray> &maps,bytearray &binarized,
                  float rsmooth=1.0,float asigma=0.7,float mpower=0.5,
                  float rpsmooth=1.0) {
        using namespace narray_ops;
        floatarray smoothed;
        smoothed = binarized;
        sub(max(smoothed),smoothed);
        brushfire_2(smoothed);
        gauss2d(smoothed,rsmooth,rsmooth);
        floatarray zero;
        zero = smoothed;
        floatarray strength;
        floatarray angle;
        horn_riley_ridges(smoothed,zero,strength,angle);
        // dshown(smoothed,"yyY");
        // dshown(zero,"yYy");
        for(int i=0;i<zero.length();i++) {
            if(zero[i]) continue;
            strength[i] = 0;
            angle[i] = 0;
            smoothed[i] = 0;
        }
        dshown(smoothed,"yYY");
        dshown(angle,"Yyy");

        for(int m=0;m<maps.length();m++) {
            float center = m*M_PI/maps.length();
            maps(m) = smoothed;
            for(int i=0;i<smoothed.length();i++) {
                if(!smoothed[i]) continue;
                float a = angle[i];
                float d = a-center;
                float dn = normorient(d);
                float v = exp(-sqr(dn/(2*asigma)));
                maps(m)[i] = pow(maps(m)[i],mpower)*v;
            }
            gauss2d(maps(m),rpsmooth,rpsmooth);
        }
    }

    void compute_troughs(floatarray &troughs,bytearray &binarized,
                         float rsmooth=1.0) {
        using namespace narray_ops;
        floatarray smoothed;
        smoothed = binarized;
        brushfire_2(smoothed);
        gauss2d(smoothed,rsmooth,rsmooth);
        floatarray zero;
        zero = smoothed;
        floatarray strength;
        floatarray angle;
        horn_riley_ridges(smoothed,zero,strength,angle);
        // dshown(smoothed,"yyY");
        // dshown(zero,"yYy");
        for(int i=0;i<zero.length();i++) {
            if(zero[i]) continue;
            strength[i] = 0;
            angle[i] = 0;
            smoothed[i] = 0;
        }
        abs(strength);
        troughs = strength;
        troughs /= max(troughs);
    }

    void extract_holes(bytearray &holes,bytearray &binarized) {
        dsection("extract_holes");
        using namespace narray_ops;

        intarray temp;
        temp.copy(binarized);

        // check whether the image is all black or all white; in that
        // case, there are no holes
        if(max(temp)==min(temp)) {
            holes.makelike(temp,0);
            return;
        }

        // clear the border so that we don't get spurious holes along
        // the border
        int w = temp.dim(0), h = temp.dim(1);
        for(int i=0;i<w;i++) temp(i,0) = 0;
        for(int i=0;i<w;i++) temp(i,h-1) = 0;
        for(int j=0;j<h;j++) temp(0,j) = 0;
        for(int j=0;j<h;j++) temp(w-1,j) = 0;

        // now invert it (since we're looking for holes)
        greater(temp,0,1,0);
        dshown(temp,"a");

        // we're assuming that the image is white characters
        // on black background; we want to label the background blobs
        // so we invert
        label_components(temp);
        dshowr(temp,"b");

        // now try to find what label has been assigned to the background
        // blob (that involves a little bit of guessing)
        int background = -1;
        for(int i=0;i<temp.dim(0);i++) {
            if(temp(i,0)!=0) {
                background = temp(i,0);
                break;
            }
        }
        CHECK(background>0);

        // finally, extract the holes--all the regions that have a label
        // different from the background
        makelike(holes,temp);
        holes = 0;
        for(int i=0;i<temp.dim(0);i++) {
            for(int j=0;j<temp.dim(1);j++) {
                if(temp(i,j)>0 && temp(i,j)!=background)
                    holes(i,j) = 255;
            }
        }

        //dshowr(temp,"Yyy");
        dshown(holes,"d");
        dwait();
    }
}
