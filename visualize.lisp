(require :sdl2)
(require :float-features)
(require :fast-io)

(defconstant +h+ 1/24 "Kernel Support Radius")
(defconstant +d+ (/ +h+ 1.2d0) "Particle spacing")

;;; Particle

(defstruct particle
  (x 0d0 :type double-float)
  (y 0d0 :type double-float)
  (pressure 0d0 :type double-float))

(defmethod print-object ((o particle) stream)
  (with-slots (x y pressure) o
    (format stream "#S(PARTICLE (~,2e, ~,2e) :pressure ~,2e)"
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
           (truncate (+ 200 (* 250 x))))
         (transform-y (y)
           (truncate (- 200 (* 250 y)))))

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

(defclass simulation ()
  ((io-buffer :initarg :io-buffer)
   (header-end-position)
   (little-endian-p :initarg :little-endian-p)
   (ended :initform nil :accessor ended)
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

(defmethod read-double ((s simulation))
  (declare (optimize (debug 3)))
  (with-slots (little-endian-p io-buffer) s
      (float-features:bits-double-float
                (if little-endian-p
                    (fast-io:readu64-le io-buffer)
                    (fast-io:readu64-be io-buffer)))))

(defun open-simulation-file (filename)
  (let* ((stream (open filename :direction :input
                                :element-type '(unsigned-byte 8)))
         (io-buffer (fast-io:make-input-buffer :stream stream))
         (little-endian-p (case (fast-io:readu8 io-buffer)
                            (1 T)
                            (0 nil)))
         (sim (make-instance 'simulation :io-buffer io-buffer :little-endian-p little-endian-p)))

    (let* ((count (read-u32 sim))
           (particles (make-array count
                                  :element-type '(or null particle)
                                  :initial-element nil)))
      (setf (slot-value sim 'header-end-position) (file-position stream))
      (loop for i from 0 below count do
        (setf (aref particles i) (make-particle :x 0.0d0
                                                :y 0.0d0
                                                :pressure 0.0d0)))
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
              (loop for p across (particles s) do
                (setf (particle-x p) (read-double s)
                      (particle-y p) (read-double s)
                      (particle-pressure p) (read-double s)))
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


(defun main ()
  (sdl2:with-init (:everything)
    ;; Initialize Graphics
    (sdl2:with-window (win :w 1000 :h 600
                           :title "Physics simulation"
                           :flags (list sdl2-ffi:+sdl-window-resizable+))
      (sdl2:with-renderer (renderer win)
        (with-simulation-file ("./out/results.data" sim)
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
                 (#.sdl2-ffi:+sdl-scancode-space+
                  (read-frame sim))
                 (#.sdl2-ffi:+sdl-scancode-r+
                  (reset-simulation sim)
                  (read-frame sim)))))
            (:idle
             ()
             ;; Clear screen
             (sdl2:set-render-draw-color renderer 255 255 255 255)
             (sdl2:render-clear renderer)
             ;; Draw objects
             (draw renderer (particles sim))
             ;; Update screnn
             (sdl2:render-present renderer))))))))
