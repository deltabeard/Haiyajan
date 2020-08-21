function targres = dpiconv(srcres, srcdpi, targdpi)
  % DPICONV Produce dimensions of an image that simulates its size when viewed
  %     on a display with the target DPI.
  narginchk(3,3);
  assert(size(srcres, 2) == 2);
  assert(size(srcdpi, 2) == 1);
  assert(size(targdpi, 2) == 1);
  
  dpi_factor = srcdpi/targdpi;
  targres = srcres .* dpi_factor;
endfunction
