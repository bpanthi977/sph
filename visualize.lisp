(require :sdl2)
(require :float-features)
(require :fast-io)

(defpackage visualize
  (:use :cl))

(in-package visualize)

(defconstant +h+ 1/24 "Kernel Support Radius")
(defconstant +d+ (/ +h+ 1.2d0) "Particle spacing")

;;; Particle

(defstruct particle
  (x 0.0 :type short-float)
  (y 0.0 :type short-float)
  (vx 0.0 :type short-float)
  (vy 0.0 :type short-float)
  (mass 0.0 :type short-float)
  (pressure 0.0 :type short-float))

(defmethod print-object ((o particle) stream)
  (with-slots (x y) o
    (format stream "#S(PARTICLE (~,2e, ~,2e))"
            x y)))

(defun draw-circle (renderer cx cy r)
  "Midpoint Circle Algorithm"
  (let* ((x r)
         (y 0)
         (err (- 3 (* 2 r))))
    (flet ((draw-point (x y)
             (funcall renderer (+ cx x) (- cy y))
             (funcall renderer (+ cx x) (+ cy y))
             (funcall renderer (- cx x) (- cy y))
             (funcall renderer (- cx x) (+ cy y))
             (funcall renderer (+ cx y) (- cy x))
             (funcall renderer (+ cx y) (+ cy x))
             (funcall renderer (- cx y) (- cy x))
             (funcall renderer (- cx y) (+ cy x))))

      (loop while (>= x y) do
        (draw-point x y)
        (incf y)
        (cond ((> err 0)
               (decf x)
               (incf err (* 2 (+ 5 (* -2 x) (* 2 y)))))
              (t
               (incf err (* 2 (+ 3 (* 2 y))))))))))

(defparameter *translate-x* 200)
(defparameter *translate-y* 200)
(defparameter *scale* 200)

(defun draw (draw-point set-color particles)
  (flet ((transform-x (x)
           ;; x is in m
           ;; take 500 pixels = 1m
           (truncate (+ *translate-x* (* *scale* x))))
         (transform-y (y)
           (truncate (- *translate-y* (* *scale* y)))))

    (loop for p across particles
          with max-pressure = (max 1 (reduce #'max particles :key #'particle-pressure :initial-value 0))
          with r = (truncate (* *scale* 1/2 +d+))
          for x = (particle-x p)
          for y = (particle-y p) do
            (funcall set-color
                     (floor (* 255 (min 1 (/ (particle-pressure p)
                                             max-pressure))))
                     0 255)
            (draw-circle draw-point (transform-x x) (transform-y y) r))))

(defconstant +SIM-LITTLE-ENDIAN+     #b00001)
(defconstant +SIM-MASS+              #b00010)
(defconstant +SIM-PRESSURE+          #b00100)
(defconstant +SIM-VELOCITY+          #b01000)

(defclass simulation ()
  ((io-buffer :initarg :io-buffer)
   (header-end-position)
   (flags :initarg :flags :accessor flags)
   (little-endian-p :initarg :little-endian-p)
   (ended :initform nil :accessor ended)
   (world-time :initform 0 :accessor world-time)
   (particles :accessor particles)
   (frame-number :accessor frame-number :initform 0)))

(defmethod read-u8 ((s simulation))
  (with-slots (io-buffer) s
    (case (fast-io:readu8 io-buffer)
      (1 T)
      (0 nil))))

(defmethod read-u32 ((s simulation))
  (with-slots (little-endian-p io-buffer) s
    (if little-endian-p
        (fast-io:read32-le io-buffer)
        (fast-io:read32-be io-buffer))))

(defmethod read-single ((s simulation))
  (declare (optimize (debug 3)))
  (with-slots (little-endian-p io-buffer) s
    (float-features:bits-single-float
     (if little-endian-p
         (fast-io:readu32-le io-buffer)
         (fast-io:readu32-be io-buffer)))))

(defun open-simulation-file (filename)
  (let* ((stream (open filename :direction :input
                                :element-type '(unsigned-byte 8)))
         (io-buffer (fast-io:make-input-buffer :stream stream))
         (flags (fast-io:readu8 io-buffer))
         (little-endian-p (case (logand +sim-little-endian+ flags)
                            (1 T)
                            (0 nil)))
         (sim (make-instance 'simulation :io-buffer io-buffer :little-endian-p little-endian-p :flags flags)))

    (let* ((count (read-u32 sim))
           (particles (make-array count
                                  :element-type '(or null particle)
                                  :initial-element nil)))
      (setf (slot-value sim 'header-end-position) (file-position stream))
      (loop for i from 0 below count do
        (setf (aref particles i) (make-particle :x 0.0
                                                :y 0.0
                                                :mass (if (= 0 (logand +sim-mass+ flags))
                                                          0.0
                                                          (read-single sim)))))
      (print flags)
      (setf (particles sim) particles)
      sim)))

(defmethod reset-simulation ((s simulation))
  (with-slots (io-buffer header-end-position) s
    (file-position (fast-io:input-buffer-stream io-buffer)
                   header-end-position)
    (setf (ended s) nil)
    (setf (frame-number s) 0)))

(defmethod read-frame ((s simulation))
  (if (ended s)
      nil
      (let ((has-next-frame (read-u8 s)))
        (if has-next-frame
            (progn
              (setf (world-time s) (read-single s))
              (loop for p across (particles s) do
                (setf (particle-x p) (read-single s)
                      (particle-y p) (read-single s)
                      (particle-pressure p) (if (= 0 (logand +sim-pressure+ (flags s)))
                                                0.0
                                                (read-single s))))
              (incf (frame-number s)))
            (setf (ended s) t))
        (not (ended s)))))

(defmethod close-simulation ((s simulation))
  (with-slots (io-buffer) s
    (close (fast-io:input-buffer-stream io-buffer))))

(defmacro with-simulation-file ((filename simulation) &body body)
  `(let ((,simulation (open-simulation-file ,filename)))
     (unwind-protect (progn ,@body)
       (close-simulation ,simulation))))

(defun gl-render (win gl sim)
  (gl:clear :color-buffer)
  (multiple-value-bind (width height) (sdl2:get-window-size win)
    (let ((buffer (make-array (list height width 3) :element-type '(unsigned-byte 8)))
          (r 0) (g 0) (b 0))
      (flet ((draw-point (x y r g b)
               (setf (aref buffer y x 0) r
                     (aref buffer y x 1) g
                     (aref buffer y x 2) b)))
        (draw (lambda (x y)
                (let ((x (floor x))
                      (y (- height (floor y))))
                  (when (and (<= 0 x (1- width))
                             (<= 0 y (1- height)))
                    (draw-point x y r g b))))
              (lambda (_r _g _b)
                (setf r _r g _g b _b))
              (particles sim))

        (gl:draw-pixels width height
                        :rgb
                        :unsigned-byte
                        (make-array (* height width 3) :element-type '(unsigned-byte 8)
                                                       :displaced-to buffer)))))
  (gl:flush)
  (sdl2:gl-swap-window win))

(defun sdl-render (win renderer sim)
  ;; Clear screen
  (sdl2:set-render-draw-color renderer 255 255 255 255)
  (sdl2:render-clear renderer)
  ;; Draw objects
  (draw (lambda (x y)
          (sdl2:render-draw-point renderer x y))
        (lambda (r g b)
          (sdl2:set-render-draw-color renderer r g b sdl2-ffi:+sdl-alpha-opaque+))
        (particles sim))
  ;; Update screnn
  (sdl2:render-present renderer))

(defun render (win sim renderer gl)
  (if nil
      (sdl-render win renderer sim)
      (gl-render win gl sim)))

(defun main ()
  (sdl2:with-init (:everything)
    ;; Initialize Graphics
    (sdl2:with-window (win :w 1000 :h 600
                           :title "Physics simulation"
                           :flags (list sdl2-ffi:+sdl-window-resizable+ sdl2-ffi:+sdl-window-opengl+))
      (sdl2:with-gl-context (gl win)
        ;;(sdl2:with-renderer (renderer win)
        (sdl2:gl-make-current win gl)
        (with-simulation-file ("./out/results.data" sim)
          (let ((render-time-start (get-internal-real-time))
                (last-frame nil)
                (auto-play nil))
            (read-frame sim)
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
                   (#.sdl2-ffi:+sdl-scancode-f+
                    (read-frame sim))
                   (#.sdl2-ffi:+sdl-scancode-space+
                    (if auto-play
                        (setf auto-play nil)
                        (progn
                          (setf auto-play t
                                render-time-start (- (get-internal-real-time)
                                                     (* internal-time-units-per-second (world-time sim)))))))
                   (#.sdl2-ffi:+sdl-scancode-r+
                    (close-simulation sim)
                    (setf sim (open-simulation-file "./out/results.data"))
                    (setf render-time-start (get-internal-real-time))
                    (read-frame sim))
                   (#.sdl2-ffi:+sdl-scancode-up+
                    (decf *translate-y* 25))
                   (#.sdl2-ffi:+sdl-scancode-down+
                    (decf *translate-y* -25))
                   (#.sdl2-ffi:+sdl-scancode-left+
                    (decf *translate-x* -25))
                   (#.sdl2-ffi:+sdl-scancode-right+
                    (decf *translate-x* 25))
                   (#.sdl2-ffi:+sdl-scancode-minus+
                    (setf *scale* (* *scale* 0.9)))
                   (#.sdl2-ffi:+sdl-scancode-equals+
                    (setf *scale* (/ *scale* 0.9))))))
              (:idle
               ()
               ;; Read frames
               (loop while
                     (and auto-play
                          (< (world-time sim)
                             (/ (- (get-internal-real-time) render-time-start)
                                internal-time-units-per-second))
                          (not (ended sim)))
                     do (read-frame sim))
               (unless (eql (world-time sim) last-frame)
                 (format t "T=~a (~f)~%" (world-time sim) (/ (- (get-internal-real-time) render-time-start)
                                                             internal-time-units-per-second))
                 (render win sim nil gl)
                 (setf last-frame (world-time sim)))
               (if (ended sim)
                   (sleep 0.1))))))))))
