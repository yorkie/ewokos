#include <graph/graph.h>
#include <graph/graph_png.h>
#include <ewoksys/klog.h>

int ewok_png_probe_result = -1;

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;
    graph_t *image = png_image_new("/usr/system/icons/xlogo.png");
    if(image == NULL || image->w == 0 || image->h == 0)
        return -1;
    klog("png_probe.wasm: decoded ext3 PNG %ux%u with libpng+zlib\n",
            image->w, image->h);
    graph_free(image);
    ewok_png_probe_result = 0;
    return 0;
}
