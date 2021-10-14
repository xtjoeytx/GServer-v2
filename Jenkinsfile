def notify(status){
	emailext (
		body: '$DEFAULT_CONTENT',
		recipientProviders: [
			[$class: 'CulpritsRecipientProvider'],
			[$class: 'DevelopersRecipientProvider'],
			[$class: 'RequesterRecipientProvider']
		],
		replyTo: '$DEFAULT_REPLYTO',
		subject: '$DEFAULT_SUBJECT',
		to: '$DEFAULT_RECIPIENTS'
	)
}

@NonCPS
def killall_jobs() {
	def jobname = env.JOB_NAME
	def buildnum = env.BUILD_NUMBER.toInteger()
	def killnums = ""
	def job = Jenkins.instance.getItemByFullName(jobname)
	def fixed_job_name = env.JOB_NAME.replace('%2F','/')

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals" }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer" }

		echo "Kill task = ${build}"

		killnums += "#" + build.getNumber().toInteger() + ", "

		build.doStop();
	}

	if (killnums != "") {
		discordSend description: "in favor of #${buildnum}, ignore following failed builds for ${killnums}", footer: "", link: env.BUILD_URL, result: "ABORTED", title: "[${split_job_name[0]}] Killing task(s) ${fixed_job_name} ${killnums}", webhookURL: env.GS2EMU_WEBHOOK
	}
	echo "Done killing"
}

def buildStep(dockerImage, os, defines) {
	def split_job_name = env.JOB_NAME.split(/\/{1}/)
	def fixed_job_name = split_job_name[1].replace('%2F',' ')
	def fixed_os = os.replace(' ','-')

	try{
		stage("Building on \"${dockerImage}\" for \"${os}\"...") {
			properties([pipelineTriggers([githubPush()])])
			def commondir = env.WORKSPACE + '/../' + fixed_job_name + '/'

			def pathInContainer

			def dockerImageRef = docker.image("${dockerImage}")
			dockerImageRef.pull()

			dockerImageRef.inside("") {
				pathInContainer = steps.sh(script: 'echo $PATH', returnStdout: true).trim()
			}

			dockerImageRef.inside("-e HOME='/tmp' --privileged") {
				checkout scm

				if (env.CHANGE_ID) {
					echo 'Trying to build pull request'
				}

				if (!env.CHANGE_ID) {
				//	sh "rm -rfv publishing/deploy/*"
				//	sh "mkdir -p publishing/deploy/gs2emu"
				}

				dir("dependencies") {
				//	sh "BUILDARCH=${arch} ./build-v8-linux"
				}

				sh "mkdir -p build/"
				sh "mkdir -p lib/"
				sh "rm -rfv build/*"

				slackSend color: "good", channel: "#jenkins", message: "Starting ${os} build target..."
				dir("build") {
					sh "cmake ${defines} -DVER_EXTRA=\"-${fixed_os}-${fixed_job_name}\" .."
					sh "cmake --build . --config Release --target package -- -j 8"
					//sh "cmake --build . --config Release --target package_source -- -j 8"

					archiveArtifacts artifacts: '*.zip,*.tar.gz,*.tgz'
				}

				discordSend description: "Target: ${os} DockerImage: ${dockerImage} Generator: ${generator} successful!", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
			}
		}
	} catch(err) {
		currentBuild.result = 'FAILURE'

		discordSend description: "Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} (<${env.BUILD_URL}|Open>)", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
		notify('Build failed')
		throw err
	}
}

def buildStepDocker(DOCKER_ROOT, DOCKERIMAGE, DOCKERTAG, DOCKERFILE, BUILD_NEXT) {
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');

	try {
		checkout scm;

		def buildenv = "${DOCKERTAG}";
		def tag = '';
		if (env.BRANCH_NAME.equals('master')) {
			tag = "latest";
		} else {
			tag = "${env.BRANCH_NAME.replace('/','-')}";
		}

		docker.withRegistry("https://index.docker.io/v1/", "dockergraal") {
			def customImage
			stage("Building ${DOCKERIMAGE}:${tag}...") {
				customImage = docker.build("${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", "--build-arg BUILDENV=${buildenv} --network=host --pull -f ${DOCKERFILE} .");
			}

			stage("Pushing to docker hub registry...") {
				customImage.push();
				discordSend description: "Docker Image: ${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
			}
		}

		if (!BUILD_NEXT.equals('')) {
			build "${BUILD_NEXT}/${env.BRANCH_NAME}";
		}
	} catch(err) {
		currentBuild.result = 'FAILURE'

		discordSend description: "", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK

		notify("Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}")
		throw err
	}
}

node('master') {
	killall_jobs();
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');
	checkout(scm);

	env.COMMIT_MSG = sh (
		script: 'git log -1 --pretty=%B ${GIT_COMMIT}',
		returnStdout: true
	).trim();

	discordSend description: "${env.COMMIT_MSG}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Started: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK

	def branches = [:]
	def project = readJSON file: "JenkinsEnv.json";

	project.builds.each { v ->
		branches["Build ${v.Title}"] = {
			node(v.OS) {
				if ("${v.Type}" == "docker") {
					buildStepDocker(v.Config.DockerRoot, v.Config.DockerImage, v.Config.DockerTag, v.Config.Dockerfile, v.Config.BuildIfSuccessful);
				} else {
					buildStep(v.Config.DockerImage, v.Title, v.Config.Flags)
				}
			}
		}
	}

	sh "rm -rf ./*"

	parallel branches;
}
