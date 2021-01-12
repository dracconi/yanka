# Taken from jpm

(def- is-win (= (os/which) :windows))
(def- is-mac (= (os/which) :macos))
(def- sep (if is-win "\\" "/"))

(defn- libjanet
  "Find libjanet.a (or libjanet.lib on windows) at compile time"
  []
  (def libpath (dyn :libpath JANET_LIBPATH))
  (unless libpath
    (error "cannot find libpath: provide --libpath or JANET_LIBPATH"))
  (string (dyn :libpath JANET_LIBPATH)
          sep
          (if is-win "libjanet.lib" "libjanet.a")))

#---


(declare-project
  :name "yanka"
  :description "Native HTTP client for Janet.")

(declare-native
  :name "ssl"
  :lflags (cond is-win 
                [;default-lflags "libcrypto.lib" "libssl.lib" "Ws2_32.lib"]
                 [;default-lflags "-lcrypto" "-lssl"])
  :source @["ssl.c"])
  
(declare-source
  :source @["yanka.janet"])
