(require :sdl2)
(require :float-features)

(defconstant +h+ 1/24 "Kernel Support Radius")
(defconstant +d+ (/ +h+ 1.2d0) "Particle spacing")

;;; Particle

(defstruct particle
  (x 0d0 :type double-float)
  (y 0d0 :type double-float)
  (pressure 0d0 :type double-float))

(defmethod print-object ((o particle) stream)
  (with-slots (x y pressure) o
    (format stream "#S(PARTICLE (~,3f, ~,4f) :pressure ~,3f)"
            x y pressure)))

(defun draw-circle (renderer cx cy r)
  "Midpoint Circle Algorithm"
  (let* ((x r)
         (y 0)
         (err (- 3 (* 2 r))))
    (flet ((draw-point (x y)
             (sdl2:render-draw-point renderer (+ cx x) (- cy y))
             (sdl2:render-draw-point renderer (+ cx x) (+ cy y))
             (sdl2:render-draw-point renderer (- cx x) (- cy y))
             (sdl2:render-draw-point renderer (- cx x) (+ cy y))
             (sdl2:render-draw-point renderer (+ cx y) (- cy x))
             (sdl2:render-draw-point renderer (+ cx y) (+ cy x))
             (sdl2:render-draw-point renderer (- cx y) (- cy x))
             (sdl2:render-draw-point renderer (- cx y) (+ cy x))))

      (loop while (>= x y) do
        (draw-point x y)
        (incf y)
        (cond ((> err 0)
               (decf x)
               (incf err (* 2 (+ 5 (* -2 x) (* 2 y)))))
              (t
               (incf err (* 2 (+ 3 (* 2 y))))))))))

(defun draw (renderer particles)
  (flet ((transform-x (x)
           ;; x is in m
           ;; take 500 pixels = 1m
           (truncate (+ 100 (* 250 x))))
         (transform-y (y)
           (truncate (- 550 (* 250 y)))))

    (loop for p across particles
          with max-pressure = (max 1 (reduce #'max particles :key #'particle-pressure :initial-value 0))
          with r = (truncate (* 250 1/2 +d+))
          for x = (particle-x p)
          for y = (particle-y p) do
            (sdl2:set-render-draw-color renderer
                                        (floor (* 255 (min 1 (/ (particle-pressure p)
                                                                max-pressure))))
                                        0 0
                                        sdl2-ffi:+sdl-alpha-opaque+)

            (draw-circle renderer (transform-x x) (transform-y y) r))))

(defun load-simulation-results0 ()
  (with-open-file (stream "out/results.data"
                          :direction :input
                          :element-type '(unsigned-byte 32))
    (flet ((read-uint32 ()
             (read-byte stream))
           (read-double ()
             (float-features:bits-double-float (read-byte stream))))
      (let* ((count (read-uint32))
             (particles (make-array count
                                    :element-type '(or null particle)
                                    :initial-element nil)))
        (loop for i from 0 below count do
          (setf (aref particles i) (make-particle :x (read-double)
                                                  :y (read-double)
                                                  :pressure (read-double))))

        particles))))

(defun load-simulation-results ()
  (restart-case (load-simulation-results0)
    (retry ()
      (load-simulation-results))
    (ignore ()
      #())))

(defun main ()
  (sdl2:with-init (:everything)
    ;; Initialize Graphics
    (sdl2:with-window (win :w 1000 :h 600
                           :title "Physics simulation"
                           :flags (list sdl2-ffi:+sdl-window-resizable+))
      (sdl2:with-renderer (renderer win)
        (let ((particles (load-simulation-results)))
          (sdl2:with-event-loop ()
            (:quit () t)
            (:window
             (:type type)
             (cond ((= sdl2-ffi:+sdl-windowevent-close+ type)
                    (sdl2:push-event :quit))))
            (:keydown
             (:keysym keysym)
             (let ((scancode (sdl2:scancode-value keysym)))
               (case scancode
                 (#.(list sdl2-ffi:+sdl-scancode-escape+ sdl2-ffi:+sdl-scancode-q+)
                  (sdl2:push-event :quit))
                 (#.sdl2-ffi:+sdl-scancode-r+
                  (setf particles (load-simulation-results))))))
            (:idle
             ()
             ;; Clear screen
             (sdl2:set-render-draw-color renderer 255 255 255 255)
             (sdl2:render-clear renderer)
             ;; Draw objects
             (draw renderer particles)
             ;; Update screnn
             (sdl2:render-present renderer))))))))
