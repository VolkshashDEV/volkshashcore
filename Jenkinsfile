// This Jenkinsfile will build a builder image and then run the actual build and tests inside this image
// It's very important to not execute any scripts outside of the builder container, as it's our protection against
// external developers bringing in harmful code into Jenkins.
// Jenkins will only run the build if this Jenkinsfile was not modified in an external pull request. Only branches
// which are part of the Volkshash repo will allow modification to the Jenkinsfile.

def targets = [
  'win32',
  'win64',
  'linux32',
  'linux64',
  'linux64_nowallet',
  'linux64_release',
  'mac',
]

def tasks = [:]
for(int i = 0; i < targets.size(); i++) {
  def target = targets[i]

  tasks["${target}"] = {
    node {
      def BUILD_NUMBER = sh(returnStdout: true, script: 'echo $BUILD_NUMBER').trim()
      def BRANCH_NAME = sh(returnStdout: true, script: 'echo $BRANCH_NAME').trim()
      def UID = sh(returnStdout: true, script: 'id -u').trim()
      def HOME = sh(returnStdout: true, script: 'echo $HOME').trim()
      def pwd = sh(returnStdout: true, script: 'pwd').trim()

      checkout scm

      def env = [
        "BUILD_TARGET=${target}",
        "PULL_REQUEST=false",
        "JOB_NUMBER=${BUILD_NUMBER}",
      ]
      withEnv(env) {
        def builderImageName="volkshash-builder-${target}"

        def builderImage
        stage("${target}/builder-image") {
          builderImage = docker.build("${builderImageName}", "--build-arg BUILD_TARGET=${target} ci -f ci/Dockerfile.builder")
        }

        builderImage.inside("-t") {
          // copy source into fixed path
          // we must build under the same path everytime as otherwise caches won't work properly
          sh "cp -ra ${pwd}/. /volkshash-src/"

          // restore cache
          def hasCache = false
          try {
            copyArtifacts(projectName: "volkshashpay-volkshash/${BRANCH_NAME}", optional: true, selector: lastSuccessful(), filter: "ci-cache-${target}.tar.gz")
          } catch (Exception e) {
          }
          if (fileExists("ci-cache-${target}.tar.gz")) {
            hasCache = true
            echo "Using cache from volkshashpay-volkshash/${BRANCH_NAME}"
          } else {
            try {
              copyArtifacts(projectName: 'volkshashpay-volkshash/develop', optional: true, selector: lastSuccessful(), filter: "ci-cache-${target}.tar.gz");
            } catch (Exception e) {
            }
            if (fileExists("ci-cache-${target}.tar.gz")) {
              hasCache = true
              echo "Using cache from volkshashpay-volkshash/develop"
            }
          }

          if (hasCache) {
            sh "cd /volkshash-src && tar xzf ${pwd}/ci-cache-${target}.tar.gz"
          } else {
            sh "mkdir -p /volkshash-src/ci-cache-${target}"
          }

          stage("${target}/depends") {
            sh 'cd /volkshash-src && ./ci/build_depends.sh'
          }
          stage("${target}/build") {
            sh 'cd /volkshash-src && ./ci/build_src.sh'
          }
          stage("${target}/test") {
            sh 'cd /volkshash-src && ./ci/test_unittests.sh'
          }
          stage("${target}/test") {
            sh 'cd /volkshash-src && ./ci/test_integrationtests.sh'
          }

          // archive cache and copy it into the jenkins workspace
          sh "cd /volkshash-src && tar czfv ci-cache-${target}.tar.gz ci-cache-${target} && cp ci-cache-${target}.tar.gz ${pwd}/"
        }

        // upload cache
        archiveArtifacts artifacts: "ci-cache-${target}.tar.gz", fingerprint: true
      }
    }
  }
}

parallel tasks

