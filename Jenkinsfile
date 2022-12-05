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
	def jobname = env.JOB_NAME;
	def buildnum = env.BUILD_NUMBER.toInteger();
	def killnums = "";
	def job = Jenkins.instance.getItemByFullName(jobname);
	def fixed_job_name = env.JOB_NAME.replace('%2F','/');
	def split_job_name = env.JOB_NAME.split(/\/{1}/);

	for (build in job.builds) {
		if (!build.isBuilding()) { continue; }
		if (buildnum == build.getNumber().toInteger()) { continue; println "equals"; }
		if (buildnum < build.getNumber().toInteger()) { continue; println "newer"; }

		echo "Kill task = ${build}";

		killnums += "#" + build.getNumber().toInteger() + ", ";

		build.doStop();
	}

	if (killnums != "") {
		discordSend description: "in favor of #${buildnum}, ignore following failed builds for ${killnums}", footer: "", link: env.BUILD_URL, result: "ABORTED", title: "[${split_job_name[0]}] Killing task(s) ${fixed_job_name} ${killnums}", webhookURL: env.GS2EMU_WEBHOOK;
	}
	echo "Done killing"
}

def buildStep(dockerImage, os, defines, DOCKERTAG) {
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');
	def fixed_os = os.replace(' ','-');

	try{
		stage("Building on \"${dockerImage}\" for \"${os}\"...") {
			properties([pipelineTriggers([githubPush()])])
			def commondir = env.WORKSPACE + '/../' + fixed_job_name + '/'

			def pathInContainer

			def dockerImageRef = docker.image("${dockerImage}");
			dockerImageRef.pull();

			dockerImageRef.inside("") {
				//pathInContainer = steps.sh(script: 'echo $PATH', returnStdout: true).trim()
			}
			checkout scm;

			def tag = '';
			def extra_ver = '';
			if (env.BRANCH_NAME.equals('master')) {
				tag = "latest${DOCKERTAG}";
			} else {
				tag = "${env.BRANCH_NAME.replace('/','-')}${DOCKERTAG}";
				extra_ver = "-${tag}";
			}

			dockerImageRef.inside("") {

				if (env.CHANGE_ID) {
					echo 'Trying to build pull request'
				}

				if (!env.CHANGE_ID) {
				//	sh "rm -rfv publishing/deploy/*"
				//	sh "mkdir -p publishing/deploy/gs2emu"
				}

				if (defines.toLowerCase().contains("v8npcserver=on")) {
					dir("dependencies") {
						sh "BUILDARCH=${arch} ./build-v8-linux"
					}
				}

				sh "rm -rfv build/*"

				sh "cmake -S. -Bbuild/ ${defines} -DCMAKE_BUILD_TYPE=Release -DVER_EXTRA=\"${extra_ver}\" -DCMAKE_CXX_FLAGS_RELEASE=\"-O3 -ffast-math\" .."
				sh "cmake --build build/ --config Release -- -j 8"
				sh "cmake --build build/ --config Release --target package -- -j 8"
				//sh "cmake --build . --config Release --target package_source -- -j 8"


				dir("build") {
					try{
						sh "ctest -T test --no-compress-output --output-on-failure"
					} catch(err) {
						currentBuild.result = 'FAILURE'

						discordSend description: "Testing Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} (<${env.BUILD_URL}|Open>)", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
						notify('Build failed')
					}
					archiveArtifacts (
						artifacts: 'Testing/**/*.xml',
						fingerprint: true
					)
					archiveArtifacts artifacts: '*.zip,*.tar.gz,*.tgz'
				}

				stage("Xunit") {
					// Process the CTest xml output with the xUnit plugin
					xunit (
						testTimeMargin: '3000',
						thresholdMode: 1,
						thresholds: [
							skipped(failureThreshold: '0'),
							failed(failureThreshold: '0')
						],
						tools: [CTest(
							pattern: 'build/Testing/**/*.xml',
							deleteOutputFiles: true,
							failIfNotNew: false,
							skipNoTestFiles: true,
							stopProcessingIfError: true
						)]
					);
				}

				// Clear the source and build dirs before next run
				deleteDir();

				discordSend description: "Target: ${os} DockerImage: ${dockerImage} successful!", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
			}
		}
	} catch(err) {
		currentBuild.result = 'FAILURE'

		discordSend description: "Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER} Target: ${os} DockerImage: ${dockerImage} (<${env.BUILD_URL}|Open>)", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Build Failed: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK
		notify('Build failed')
		throw err
	}
}

def buildStepDocker(DOCKER_ROOT, DOCKERIMAGE, DOCKERTAG, DOCKERFILE, BUILD_NEXT, BUILDENV) {
	def split_job_name = env.JOB_NAME.split(/\/{1}/);
	def fixed_job_name = split_job_name[1].replace('%2F',' ');

	try {
		checkout scm;

		def buildenv = "${DOCKERTAG}";
		def tag = '';
		def EXTRA_VER = '';
		def PUSH_IMAGE = BUILD_NEXT.equals('image');
		def PUSH_ARTIFACT = BUILD_NEXT.equals('artifact');

		if (BUILD_NEXT.equals('both')) {
			PUSH_IMAGE = BUILD_NEXT.equals('both');
			PUSH_ARTIFACT = BUILD_NEXT.equals('both');
		}

		if (env.BRANCH_NAME.equals('master')) {
			tag = "latest${DOCKERTAG}";
		} else {
			tag = "${env.BRANCH_NAME.replace('/','-')}${DOCKERTAG}";
			EXTRA_VER = "--build-arg VER_EXTRA=-${tag}";
		}

		docker.withRegistry("https://index.docker.io/v1/", "dockergraal") {
			def release_name = env.JOB_NAME.replace('%2F','/');
			def release_type = ("${release_name}").replace('/','-').replace('GServer-v2-','').replace('master','').replace('dev','');

			def customImage
			stage("Building ${DOCKERIMAGE}:${tag}...") {
				customImage = docker.build("${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", "--build-arg BUILDENV=${buildenv} ${EXTRA_VER} ${BUILDENV} --network=host --pull -f ${DOCKERFILE} .");
			}

			def archive_date = sh (
				script: 'date +"-%Y%m%d-%H%M"',
				returnStdout: true
			).trim();

			if (env.TAG_NAME) {
				archive_date = '';
			}

			if (PUSH_IMAGE) {
				stage("Pushing to docker hub registry...") {
					customImage.push();
					discordSend description: "Docker Image: ${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Image Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK;
				}
			}

			if (PUSH_ARTIFACT) {
				stage("Archiving artifacts...") {
					customImage.inside("") {
						sh "mkdir -p ./dist && cp -fvr /dist/* ./dist"

						dir("./dist") {
							archiveArtifacts artifacts: '*.zip,*.tar.gz,*.tgz', allowEmptyArchive: true
							discordSend description: "Docker Image: ${DOCKER_ROOT}/${DOCKERIMAGE}:${tag}", footer: "", link: env.BUILD_URL, result: currentBuild.currentResult, title: "[${split_job_name[0]}] Artifact Successful: ${fixed_job_name} #${env.BUILD_NUMBER}", webhookURL: env.GS2EMU_WEBHOOK;
						}
					}
					def dockerImageRef = docker.image("amigadev/docker-base:latest");
					dockerImageRef.pull();

					dockerImageRef.inside("") {

						stage("Github Release") {
							withCredentials([string(credentialsId: 'PREAGONAL_GITHUB_TOKEN', variable: 'GITHUB_TOKEN')]) {
								dir("./dist") {
									if (!env.CHANGE_ID) { // Don't run on PR's
										def release_type_tag = 'develop';
										def pre_release = '--pre-release';
										if (env.TAG_NAME) {
											pre_release = '';
											release_type_tag = env.TAG_NAME;
										} else if (env.BRANCH_NAME.equals('master')) {
											release_type_tag = 'nightly';
										}

										def files = sh(returnStdout: true, script: 'find . -name "*.zip" -o -name "*.tar.gz"');
										files = sh (script: "basename ${files}",returnStdout:true).trim()

										echo "${files}"

										try {
											sh "github-release release --user xtjoeytx --repo GServer-v2 --tag ${release_type_tag} --name \"GS2Emu ${release_type_tag}\" --description \"${release_type_tag} releases\" ${pre_release}"
										} catch(err) {

										}
										sh "github-release upload --user xtjoeytx --repo GServer-v2 --tag ${release_type_tag} --name \"${files}\" --file ${files} --replace"
									}
								}
							}
						}
					}
				}
			} else {
				// Do nothing
			}
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
					buildStepDocker(v.Config.DockerRoot, v.Config.DockerImage, v.Config.Tag, v.Config.Dockerfile, v.Config.BuildIfSuccessful, v.Config.BuildEnv);
				} else {
					buildStep(v.Config.DockerImage, v.OS, v.Config.Flags, v.Config.Tag)
				}
			}
		}
	}

	sh "rm -rf ./*"

	parallel branches;
}
